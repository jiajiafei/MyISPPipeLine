#include"RNR.h"
#include<math.h>
#include <stdio.h>
#include<stdlib.h>
#include <vector>
#include <thread>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <iostream>

#include <functional>
#include <cstring>
#include <atomic>

#define CLIP(x,minv,maxv) ((x)<(minv)?(minv):((x)>(maxv)?(maxv):(x)))

inline int idx(int x, int y, int w) { return y * w + x; }

// ---------------- Noise model (σ²(y) = a*y + b) ----------------
// You should fit a,b per channel with calibration images.
// Here we provide a stub and a simple interface.
struct NoiseModel {
    float a; // Poisson coeff
    float b; // Gaussian variance
};

NoiseModel fitNoiseModel_stub(/* calibration data placeholder */) {
    // Replace with real calibration: capture flat patches at different exposures/ISO,
    // compute mean/var pairs and linear-fit var = a*mean + b
    return { 1, 2 }; // example numbers, change after calibration
}

// ---------------- Generalized Anscombe VST for Poisson+Gaussian ----------------
// Use simple Anscombe variant for mixed noise. For high accuracy use M-A Anscombe (not implemented).
inline void vst_forward_anscombe(const u16* src, int n, float* dst, const NoiseModel& nm) {
    // For mixed model σ^2 = a*y + b, scale to reduce to approx Poisson, then apply Anscombe.
    // Simple transform: G = 2 * sqrt(a*y + 3/8 + b/a)  (approx). This is heuristic - tune per camera.
    float a = nm.a, b = nm.b;
    if (a <= 0.0f) a = 1e-6f;
    float offset = 0.375f + b / a;
    float factor = 2.0f * sqrtf(a);
#pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        float y = float(src[i]);
        // guard
        float v = factor * sqrtf(max(0.0f, y + offset));
        dst[i] = v;
    }
}

inline void vst_inverse_anscombe(const float* G, int n, u16* dst, const NoiseModel& nm) {
    float a = nm.a, b = nm.b;
    if (a <= 0.0f) a = 1e-6f;
    float offset = 0.375f + b / a;
#pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        float g = G[i];
        float yhat = (g / (2.0f * sqrtf(a))) * (g / (2.0f * sqrtf(a))) - offset;
        if (yhat < 0.0f) yhat = 0.0f;
        // clip to 16-bit
        dst[i] = u16(CLIP((int)(yhat + 0.5f), 0, 65535));
    }
}

// ---------------- Integral images (for fast mean/var on patches) ----------------

inline double boxSum(const std::vector<double>& integral, int W1, int x1, int y1, int x2, int y2) {
    x1 = max(0, min(x1, W1 - 1));
    x2 = max(0, min(x2, W1 - 1));
    int H1 = (int)(integral.size() / W1);
    y1 = max(0, min(y1, H1 - 1));
    y2 = max(0, min(y2, H1 - 1));
    return integral[y2 * W1 + x2] - integral[y1 * W1 + x2] - integral[y2 * W1 + x1] + integral[y1 * W1 + x1];
}
// 你提供的高效双积分图计算函数
void computeIntegralImagesFloat(const float* src, int w, int h,
    std::vector<double>& integral, std::vector<double>& integralSq) {
    integral.assign((w + 1) * (h + 1), 0.0);
    integralSq.assign((w + 1) * (h + 1), 0.0);
    int W1 = w + 1;
    for (int y = 1; y <= h; ++y) {
        double sumRow = 0.0, sumRowSq = 0.0;
        for (int x = 1; x <= w; ++x) {
            float val = src[idx(x - 1, y - 1, w)];
            sumRow += val;
            sumRowSq += double(val) * double(val);
            int id = y * W1 + x;
            integral[id] = integral[(y - 1) * W1 + x] + sumRow;
            integralSq[id] = integralSq[(y - 1) * W1 + x] + sumRowSq;
        }
    }
}

// ---------------- Patch shapes helpers ----------------
// return list of offsets (dx,dy) for diamond (Manhattan) radius r
static inline void makeDiamondOffsets(int r, std::vector<std::pair<int, int>>& offs) {
    offs.clear();
    for (int dy = -r; dy <= r; ++dy) {
        int rx = r - std::abs(dy);
        for (int dx = -rx; dx <= rx; ++dx) offs.emplace_back(dx, dy);
    }
}
// rectangle offsets radius rx,ry
static inline void makeRectOffsets(int rx, int ry, std::vector<std::pair<int, int>>& offs) {
    offs.clear();
    for (int dy = -ry; dy <= ry; ++dy)
        for (int dx = -rx; dx <= rx; ++dx)
            offs.emplace_back(dx, dy);
}
// 1. 边界镜像映射：替换原来的 std::max(0, std::min(w-1, x)) 或 CLIP
inline int reflect_border(int p, int len) {
    if (len <= 1) return 0;
    while (p < 0 || p >= len) {
        if (p < 0) p = -p;
        if (p >= len) p = 2 * len - 2 - p;
    }
    return p;
}

// 2. 边界 Fade-Out 衰减 Mask：防止边界低通滤波带来的外扩条纹
inline float compute_border_weight(int x, int y, int w, int h, int border_margin = 8) {
    int dist_x = min(x, w - 1 - x);
    int dist_y = min(y, h - 1 - y);
    int min_dist = min(dist_x, dist_y);

    if (min_dist >= border_margin) return 1.0f;

    // Smoothstep 平滑渐变 (0.0 ~ 1.0)
    float t = static_cast<float>(min_dist) / static_cast<float>(border_margin);
    return t * t * (3.0f - 2.0f * t);
}


// ---------------- Bayer split/merge (R Gr Gb B) ----------------
void splitBayer(const u16* raw, int rows, int cols,
    u16* R, u16* Gr, u16* Gb, u16* B) {
    int h2 = rows / 2, w2 = cols / 2;
    for (int i = 0; i < h2; ++i) {
        for (int j = 0; j < w2; ++j) {
            R[i * w2 + j] = raw[2 * i * cols + 2 * j];
            Gr[i * w2 + j] = raw[2 * i * cols + 2 * j + 1];
            Gb[i * w2 + j] = raw[(2 * i + 1) * cols + 2 * j];
            B[i * w2 + j] = raw[(2 * i + 1) * cols + 2 * j + 1];
        }
    }
}
void mergeBayer(u16* raw, int rows, int cols,
    const u16* R, const u16* Gr, const u16* Gb, const u16* B) {
    int h2 = rows / 2, w2 = cols / 2;
    for (int i = 0; i < h2; ++i) {
        for (int j = 0; j < w2; ++j) {
            raw[2 * i * cols + 2 * j] = R[i * w2 + j];
            raw[2 * i * cols + 2 * j + 1] = Gr[i * w2 + j];
            raw[(2 * i + 1) * cols + 2 * j] = Gb[i * w2 + j];
            raw[(2 * i + 1) * cols + 2 * j + 1] = B[i * w2 + j];
        }
    }
}
inline void splitBayerFloat(const float* src, int rows, int cols, float* R, float* Gr, float* Gb, float* B) {
    int h2 = rows / 2, w2 = cols / 2;
#pragma omp parallel for collapse(2)
    for (int r = 0; r < h2; ++r) {
        for (int c = 0; c < w2; ++c) {
            int src_r = r * 2;
            int src_c = c * 2;
            int idx_out = r * w2 + c;

            // RGGB 阵列映射
            R[idx_out] = src[src_r * cols + src_c];
            Gr[idx_out] = src[src_r * cols + (src_c + 1)];
            Gb[idx_out] = src[(src_r + 1) * cols + src_c];
            B[idx_out] = src[(src_r + 1) * cols + (src_c + 1)];
        }
    }
}

// Bayer 合并 (Float 空间)
inline void mergeBayerFloat(float* dst, int rows, int cols, const float* R, const float* Gr, const float* Gb, const float* B) {
    int h2 = rows / 2, w2 = cols / 2;
#pragma omp parallel for collapse(2)
    for (int r = 0; r < h2; ++r) {
        for (int c = 0; c < w2; ++c) {
            int dst_r = r * 2;
            int dst_c = c * 2;
            int idx_in = r * w2 + c;

            dst[dst_r * cols + dst_c] = R[idx_in];
            dst[dst_r * cols + (dst_c + 1)] = Gr[idx_in];
            dst[(dst_r + 1) * cols + dst_c] = Gb[idx_in];
            dst[(dst_r + 1) * cols + (dst_c + 1)] = B[idx_in];
        }
    }
}


// 2x2 均值下采样（硬件上对应平铺累加，极其轻量）
// 带有 3x3 高斯抗混叠 (Binomial [1,2,1]) + 镜像边界处理的 2x 下采样
void downsample2x(const float* src, float* dst, int w, int h) {
    int w2 = w / 2;
    int h2 = h / 2;

    // 二项式 [1, 2, 1] 二维核 (加权和为 16)
    //  1  2  1
    //  2  4  2
    //  1  2  1
#pragma omp parallel for collapse(2)
    for (int y2 = 0; y2 < h2; ++y2) {
        for (int x2 = 0; x2 < w2; ++x2) {
            // 对应原图 2x2 中心
            int x = x2 * 2;
            int y = y2 * 2;

            float sum = 0.0f;

            // 3x3 邻域高斯平滑 (使用 reflect_border 防边界扩展)
            for (int dy = -1; dy <= 1; ++dy) {
                int py = reflect_border(y + dy, h);
                float wy = (dy == 0) ? 2.0f : 1.0f;

                for (int dx = -1; dx <= 1; ++dx) {
                    int px = reflect_border(x + dx, w);
                    float wx = (dx == 0) ? 2.0f : 1.0f;

                    sum += src[py * w + px] * (wx * wy);
                }
            }

            dst[y2 * w2 + x2] = sum * (1.0f / 16.0f); // 归一化
        }
    }
}
// 极速版 2x 1D 可分离 Cubic 上采样 (基于 [-1, 9, 9, -1] / 16 模板)
// 性能比通用 2D Bicubic 快 5~8 倍，完全消除 Bilinear 网格折痕
void fastBicubicUpsample2x(const float* src, float* dst, int w2, int h2, int w, int h) {
    // 临时缓冲区：存储水平方向扩充后的结果 (w x h2)
    std::vector<float> tmp(w * h2);

    // Step 1: 水平方向 1D 插值 (4-tap Catmull-Rom: [-1, 9, 9, -1] / 16)
#pragma omp parallel for collapse(2)
    for (int y = 0; y < h2; ++y) {
        for (int x = 0; x < w2; ++x) {
            int x_m1 = max(0, x - 1);
            int x_p1 = min(w2 - 1, x + 1);
            int x_p2 = min(w2 - 1, x + 2);

            float p0 = src[y * w2 + x_m1];
            float p1 = src[y * w2 + x];
            float p2 = src[y * w2 + x_p1];
            float p3 = src[y * w2 + x_p2];

            // 偶数列直接复制原点
            tmp[y * w + (2 * x)] = p1;
            // 奇数列采用固定 4-tap 插值: (-p0 + 9*p1 + 9*p2 - p3) / 16
            tmp[y * w + (2 * x + 1)] = (-p0 + 9.0f * p1 + 9.0f * p2 - p3) * 0.0625f;
        }
    }

    // Step 2: 垂直方向 1D 插值
#pragma omp parallel for collapse(2)
    for (int y = 0; y < h2; ++y) {
        for (int x = 0; x < w; ++x) {
            int y_m1 = max(0, y - 1);
            int y_p1 = min(h2 - 1, y + 1);
            int y_p2 = min(h2 - 1, y + 2);

            float p0 = tmp[y_m1 * w + x];
            float p1 = tmp[y * w + x];
            float p2 = tmp[y_p1 * w + x];
            float p3 = tmp[y_p2 * w + x];

            // 偶数行直接复制
            dst[(2 * y) * w + x] = p1;
            // 奇数行采用固定 4-tap 插值
            dst[(2 * y + 1) * w + x] = (-p0 + 9.0f * p1 + 9.0f * p2 - p3) * 0.0625f;
        }
    }
}
// 完美的硬件友好型双线性上采样（带边界保护）
void bilinearUpsample(const float* src, float* dst, int src_w, int src_h, int dst_w, int dst_h) {
    float scale_x = (float)src_w / dst_w;
    float scale_y = (float)src_h / dst_h;
    for (int y = 0; y < dst_h; ++y) {
        float fy = (y + 0.5f) * scale_y - 0.5f;
        int sy = max(0, min((int)std::floor(fy), src_h - 2));
        float ty = fy - sy;

        for (int x = 0; x < dst_w; ++x) {
            float fx = (x + 0.5f) * scale_x - 0.5f;
            int sx = max(0, min((int)std::floor(fx), src_w - 2));
            float tx = fx - sx;

            float c00 = src[idx(sx, sy, src_w)];
            float c10 = src[idx(sx + 1, sy, src_w)];
            float c01 = src[idx(sx, sy + 1, src_w)];
            float c11 = src[idx(sx + 1, sy + 1, src_w)];

            dst[idx(x, y, dst_w)] = (1.0f - tx) * (1.0f - ty) * c00 +
                tx * (1.0f - ty) * c10 +
                (1.0f - tx) * ty * c01 +
                tx * ty * c11;
        }
    }
}

// ==================== 2. 基于你原有代码改造的单通道快速 NLM ====================
// 注：因为在下采样图上运行，此处将原内部的 step 强制设为 1，确保低频去噪质量最大化且速度依然极快
void nlm_channel_fast_lowres(const float* src, float* dst, int w, int h,
    int patchRadiusX, int patchRadiusY, int searchRadius, float h_param, int numThreads)
{
    const int step = 1; // 运行于低分图，step=1 质量最好且完全不卡性能
    int padX = patchRadiusX, padY = patchRadiusY;
    int Npix = w * h;
    double h2 = double(h_param) * double(h_param);

    struct Patch { int x1, y1, x2, y2; double mean, var, N; };
    std::vector<double> integral, integralSq;
    computeIntegralImagesFloat(src, w, h, integral, integralSq);
    int W1 = w + 1;

    std::vector<Patch> patchInfo(Npix);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int x1 = max(0, x - padX);
            int y1 = max(0, y - padY);
            int x2 = min(w - 1, x + padX);
            int y2 = min(h - 1, y + padY);
            double sum = boxSum(integral, W1, x1, y1, x2, y2);
            double sumSq = boxSum(integralSq, W1, x1, y1, x2, y2);
            double N = double((x2 - x1 + 1) * (y2 - y1 + 1));
            double mean = sum / N;
            double var = sumSq / N - mean * mean;
            if (var < 0) var = 0;
            patchInfo[idx(x, y, w)] = { x1, y1, x2, y2, mean, var, N };
        }
    }

    std::vector<double> weightSum(Npix, 0.0);
    std::vector<double> pixelSum(Npix, 0.0);

    int tileH = max(1, h / numThreads);
    std::vector<std::thread> ths;

    for (int t = 0; t < numThreads; ++t) {
        int y0 = t * tileH;
        int y1 = (t == numThreads - 1) ? h : (y0 + tileH);

        ths.emplace_back([=, &src, &patchInfo, &integral, &integralSq, &weightSum, &pixelSum]() {
            std::vector<double> integralDiff((w + 1) * (h + 1), 0.0);

            for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
                for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
                    std::fill(integralDiff.begin(), integralDiff.end(), 0.0);
                    for (int yy = 0; yy < h; ++yy) {
                        double rowSum = 0.0;
                        int baseId = (yy + 1) * (w + 1);
                        for (int xx = 0; xx < w; ++xx) {
                            int qx = xx + dx;
                            int qy = yy + dy;
                            double d2 = 0.0;
                            if (qx >= 0 && qx < w && qy >= 0 && qy < h) {
                                double diff = double(src[idx(xx, yy, w)]) - double(src[idx(qx, qy, w)]);
                                d2 = diff * diff;
                            }
                            rowSum += d2;
                            int id = baseId + (xx + 1);
                            integralDiff[id] = integralDiff[id - (w + 1)] + rowSum;
                        }
                    }

                    for (int yy = y0; yy < y1; yy += step) {
                        for (int xx = 0; xx < w; xx += step) {
                            int idx0 = idx(xx, yy, w);
                            const Patch& refPatch = patchInfo[idx0];

                            int sx = xx + dx, sy = yy + dy;
                            if (sx < 0 || sx >= w || sy < 0 || sy >= h) continue;

                            int nx1 = max(0, sx - padX);
                            int ny1 = max(0, sy - padY);
                            int nx2 = min(w - 1, sx + padX);
                            int ny2 = min(h - 1, sy + padY);
                            double sumN = boxSum(integral, W1, nx1, ny1, nx2, ny2);
                            double sumSqN = boxSum(integralSq, W1, nx1, ny1, nx2, ny2);
                            double Nn = double((nx2 - nx1 + 1) * (ny2 - ny1 + 1));
                            double meanN = sumN / Nn;
                            double varN = sumSqN / Nn - meanN * meanN;
                            if (varN < 0) varN = 0;

                            double meanDiff = refPatch.mean - meanN;
                            if (meanDiff * meanDiff > 50.0 + refPatch.var + varN) continue;

                            double ssd = boxSum(integralDiff, W1, refPatch.x1, refPatch.y1, refPatch.x2, refPatch.y2);
                            double dist2 = ssd / refPatch.N;
                            double wgt = std::exp(-dist2 / h2);

                            double neiVal = double(src[idx(sx, sy, w)]);

                            weightSum[idx0] += wgt;
                            pixelSum[idx0] += wgt * neiVal;
                        }
                    }
                }
            }
            });
    }

    for (auto& th : ths) if (th.joinable()) th.join();

    for (int y = 0; y < h; y += step) {
        for (int x = 0; x < w; x += step) {
            int id = idx(x, y, w);
            if (weightSum[id] > 0.0) {
                dst[id] = float(pixelSum[id] / weightSum[id]);
            }
            else {
                dst[id] = src[id];
            }
        }
    }
}
// =========================================================================
// 1. 在 NLM 主函数上方定义这个 Helper 函数
// =========================================================================
inline float compute_patch_dist_gaussian(const float* guide,
    int x1, int y1, int x2, int y2,
    int w, int h) {
    // 3x3 高斯空间加权核 (旋转对称，消除矩形 Patch 边缘滑动阶跃)
    static const float kernel[3][3] = {
        {0.0625f, 0.125f, 0.0625f},
        {0.125f,  0.250f, 0.125f},
        {0.0625f, 0.125f, 0.0625f}
    };

    float dist = 0.0f;
    for (int dy = -1; dy <= 1; ++dy) {
        int py1 = CLIP(y1 + dy, 0, h - 1);
        int py2 = CLIP(y2 + dy, 0, h - 1);
        for (int dx = -1; dx <= 1; ++dx) {
            int px1 = CLIP(x1 + dx, 0, w - 1);
            int px2 = CLIP(x2 + dx, 0, w - 1);

            float diff = guide[py1 * w + px1] - guide[py2 * w + px2];
            dist += (diff * diff) * kernel[dy + 1][dx + 1]; // 带高斯平滑的 Patch 距离
        }
    }
    return dist;
}

// =========================================================================
// 2. NLM 滤波主逻辑
// =========================================================================
void nlm_channel_fast_lowres_joint(const float* src, const float* guide, float* dst,
    int w, int h, int patch_radius, int search_radius,
    float h_param, int numThreads) {
    float h2_inv = 1.0f / (h_param * h_param + 1e-6f);

#pragma omp parallel for num_threads(numThreads) collapse(2)
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float sum_weight = 0.0f;
            float sum_val = 0.0f;

            // 搜索窗口循环 (例如 5x5 搜索区域)
            for (int sy = -search_radius; sy <= search_radius; ++sy) {
                int qy = CLIP(y + sy, 0, h - 1);
                for (int sx = -search_radius; sx <= search_radius; ++sx) {
                    int qx = CLIP(x + sx, 0, w - 1);

                    // -------------------------------------------------------------
                    // 📍【替换点】：调用 compute_patch_dist_gaussian 算两点间的 Patch 距离
                    // -------------------------------------------------------------
                    float patch_dist = compute_patch_dist_gaussian(guide, x, y, qx, qy, w, h);

                    // 计算指数权重
                    float weight = std::exp(-patch_dist * h2_inv);

                    sum_weight += weight;
                    sum_val += src[qy * w + qx] * weight;
                }
            }

            // 归一化输出
            dst[y * w + x] = sum_val / (sum_weight + 1e-6f);
        }
    }
}
void nlm_lowres_joint_fast(const float* src_low, const float* guide_low, float* dst_low,
    int w2, int h2, float h_vst, int search_radius, int numThreads) {
    float h2_inv = 1.0f / (h_vst * h_vst + 1e-6f);

#pragma omp parallel for num_threads(numThreads) collapse(2)
    for (int y = 0; y < h2; ++y) {
        for (int x = 0; x < w2; ++x) {
            float sum_weight = 0.0f;
            float sum_val = 0.0f;

            for (int sy = -search_radius; sy <= search_radius; ++sy) {
                int qy = reflect_border(y + sy, h2);
                for (int sx = -search_radius; sx <= search_radius; ++sx) {
                    int qx = reflect_border(x + sx, w2);

                    float dist = compute_patch_dist_gaussian(guide_low, x, y, qx, qy, w2, h2);
                    float weight = std::exp(-dist * h2_inv);

                    sum_weight += weight;
                    sum_val += src_low[qy * w2 + qx] * weight;
                }
            }
            dst_low[y * w2 + x] = sum_val / (sum_weight + 1e-6f);
        }
    }
}
// 硬件极其友好的高频 Coring (死区 threshold) 函数
// 平滑连续 Coring 算子 (无阈值折点，极度平滑)
inline float apply_smooth_coring(float hf_val, float threshold) {
    float x2 = hf_val * hf_val;
    float t2 = threshold * threshold;

    // f(x) = x^3 / (x^2 + T^2)
    return (hf_val * x2) / (x2 + t2 + 1e-6f);
}
inline float compute_local_variance(const float* img, int x, int y, int w, int h) {
    float sum = 0.0f, sum_sq = 0.0f;
    for (int dy = -1; dy <= 1; ++dy) {
        int py = CLIP(y + dy, 0, h - 1);
        for (int dx = -1; dx <= 1; ++dx) {
            int px = CLIP(x + dx, 0, w - 1);
            float v = img[py * w + px];
            sum += v;
            sum_sq += v * v;
        }
    }
    float mean = sum / 9.0f;
    return max(0.0f, (sum_sq / 9.0f) - mean * mean);
}
// 旋转对称的 3x3 Sobel 各向同性梯度 (消除了正交轴向偏向)
inline float compute_local_gradient_sobel(const float* img, int x, int y, int w, int h) {
    int x0 = max(0, x - 1), x1 = x, x2 = min(w - 1, x + 1);
    int y0 = max(0, y - 1), y1 = y, y2 = min(h - 1, y + 1);

    float p00 = img[y0 * w + x0], p01 = img[y0 * w + x1], p02 = img[y0 * w + x2];
    float p10 = img[y1 * w + x0], p12 = img[y1 * w + x2];
    float p20 = img[y2 * w + x0], p21 = img[y2 * w + x1], p22 = img[y2 * w + x2];

    // 水平与垂直 Sobel 卷积
    float gx = (p02 + 2.0f * p12 + p22) - (p00 + 2.0f * p10 + p20);
    float gy = (p20 + 2.0f * p21 + p22) - (p00 + 2.0f * p01 + p02);

    // 欧氏距离 (旋转不变性)，* 0.125f 进行幅值归一化
    return std::sqrt(gx * gx + gy * gy) * 0.125f;
}
// 1D 可分离 5x5 高斯低通 (完全替代 [下采样+上采样]，零混叠、零摩尔纹、零波浪涟漪)
inline void fast_lowpass_5x5(const float* src, float* dst, int w, int h) {
    std::vector<float> tmp(w * h);
    // 1D 水平高斯 [1, 4, 6, 4, 1] / 16
#pragma omp parallel for
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int x_m2 = max(0, x - 2), x_m1 = max(0, x - 1);
            int x_p1 = min(w - 1, x + 1), x_p2 = min(w - 1, x + 2);
            tmp[y * w + x] = (src[y * w + x_m2] + 4.0f * src[y * w + x_m1]
                + 6.0f * src[y * w + x]
                + 4.0f * src[y * w + x_p1] + src[y * w + x_p2]) * 0.0625f;
        }
    }
    // 1D 垂直高斯 [1, 4, 6, 4, 1] / 16
#pragma omp parallel for
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int y_m2 = max(0, y - 2), y_m1 = max(0, y - 1);
            int y_p1 = min(h - 1, y + 1), y_p2 = min(h - 1, y + 2);
            dst[y * w + x] = (tmp[y_m2 * w + x] + 4.0f * tmp[y_m1 * w + x]
                + 6.0f * tmp[y * w + x]
                + 4.0f * tmp[y_p1 * w + x] + tmp[y_p2 * w + x]) * 0.0625f;
        }
    }
}
void process_channel_vst_twoband_edge(const float* src_vst, const float* guide_vst, float* dst_vst,
    int w, int h, float h_vst, float coring_thresh_vst,
    float edge_thresh_vst, int numThreads, float nr_alpha) {
    int N = w * h;
    int w2 = w / 2, h2 = h / 2;
    int N2 = w2 * h2;
   
    // Step 1: 高斯抗混叠下采样得到低频基底
    std::vector<float> src_low(N2), guide_low(N2);
    downsample2x(src_vst, src_low.data(), w, h);
    downsample2x(guide_vst, guide_low.data(), w, h);

    // Step 2: 低频域跑 Joint-NLM (带高斯圆角 Patch)
    std::vector<float> low_clean_sub(N2);
    nlm_lowres_joint_fast(src_low.data(), guide_low.data(), low_clean_sub.data(), w2, h2, h_vst, 2, numThreads);

    // Step 3: 低频上采样回全分辨率
    std::vector<float> full_low_I(N), full_low_clean(N);
    fastBicubicUpsample2x(src_low.data(), full_low_I.data(), w2, h2, w, h);
    fastBicubicUpsample2x(low_clean_sub.data(), full_low_clean.data(), w2, h2, w, h);

    // Step 4: 两频带融合与边缘/纹理保护
#pragma omp parallel for num_threads(numThreads)
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;

            float grad = compute_local_gradient_sobel(guide_vst, x, y, w, h);
            float var = compute_local_variance(guide_vst, x, y, w, h);

            // 纹理区压制系数
            float texture_suppress = 1.0f / (1.0f + 2.0f * max(0.0f, var - 1.2f));

            // 平滑区混合 alpha
            float alpha = 1.0f - min(1.0f, grad / (edge_thresh_vst + 1e-5f));
            alpha = alpha * alpha * texture_suppress;

            // 【关键修正 3】：边界 Fade-Out 保护，防止边界外扩条纹
            float border_w = compute_border_weight(x, y, w, h, 8);
            alpha *= border_w;

            // 高频 Coring 提取
            float high_freq = src_vst[idx] - full_low_I[idx];
            float high_freq_clean = apply_smooth_coring(high_freq, coring_thresh_vst);

            // 低频修正量 Delta
            float delta = full_low_clean[idx] - full_low_I[idx];

            // 最终输出
            dst_vst[idx] = src_vst[idx] + alpha * (delta + high_freq_clean - high_freq);
            dst_vst[idx] = dst_vst[idx] * nr_alpha + (1 - nr_alpha) * src_vst[idx];
        }
    }
}


// ==================== 5. 总流水线框架对接与入口函数 ====================
// =================================================================
// 顶层 Bayer 降噪 Pipeline (集成 VST + G_mean 统一引导 + 抗混叠)
// =================================================================
void denoise_bayer_vst_twoband_pipeline(const u16* src, u16* dst, int rows, int cols,
    const NoiseModel& nm, int numThreadsAll,
    float h_r_vst, float h_g_vst, float h_b_vst,
    float coring_thresh_vst, float edge_thresh_vst,float nr_alpha) {
    int N_full = rows * cols;
    int h2 = rows / 2, w2 = cols / 2;
    int N_sub = w2 * h2;

    // -----------------------------------------------------------------
    // Step 1: 全图 Forward VST 变换
    // -----------------------------------------------------------------
    std::vector<float> vst_full_src(N_full);
    vst_forward_anscombe(src, N_full, vst_full_src.data(), nm);

    // -----------------------------------------------------------------
    // Step 2: 拆分 Bayer 四通道 (Float 空间)
    // -----------------------------------------------------------------
    std::vector<float> R_src(N_sub), Gr_src(N_sub), Gb_src(N_sub), B_src(N_sub);
    std::vector<float> R_dst(N_sub), Gr_dst(N_sub), Gb_dst(N_sub), B_dst(N_sub);

    splitBayerFloat(vst_full_src.data(), rows, cols, R_src.data(), Gr_src.data(), Gb_src.data(), B_src.data());

    // -----------------------------------------------------------------
    // 【关键修正 4】：合成 G_mean 通道，作为全局统一的 Guide
    // -----------------------------------------------------------------
    std::vector<float> G_mean_src(N_sub);
#pragma omp parallel for
    for (int i = 0; i < N_sub; ++i) {
        G_mean_src[i] = 0.5f * (Gr_src[i] + Gb_src[i]);
    }

    // -----------------------------------------------------------------
    // Step 3: 多线程并行处理 4 个通道 (全部以 G_mean_src 作为引导图)
    // -----------------------------------------------------------------
    int perChanThreads = max(1, numThreadsAll / 4);

    std::thread tR([&]() {
        process_channel_vst_twoband_edge(R_src.data(), G_mean_src.data(), R_dst.data(),
            w2, h2, h_r_vst, coring_thresh_vst, edge_thresh_vst, perChanThreads, nr_alpha);
        });
    std::thread tGr([&]() {
        process_channel_vst_twoband_edge(Gr_src.data(), G_mean_src.data(), Gr_dst.data(),
            w2, h2, h_g_vst, coring_thresh_vst, edge_thresh_vst, perChanThreads, nr_alpha);
        });
    std::thread tGb([&]() {
        process_channel_vst_twoband_edge(Gb_src.data(), G_mean_src.data(), Gb_dst.data(),
            w2, h2, h_g_vst, coring_thresh_vst, edge_thresh_vst, perChanThreads, nr_alpha);
        });
    std::thread tB([&]() {
        process_channel_vst_twoband_edge(B_src.data(), G_mean_src.data(), B_dst.data(),
            w2, h2, h_b_vst, coring_thresh_vst, edge_thresh_vst, perChanThreads, nr_alpha);
        });

    tR.join(); tGr.join(); tGb.join(); tB.join();

    // -----------------------------------------------------------------
    // Step 4: 合并四通道
    // -----------------------------------------------------------------
    std::vector<float> vst_full_dst(N_full);
    mergeBayerFloat(vst_full_dst.data(), rows, cols, R_dst.data(), Gr_dst.data(), Gb_dst.data(), B_dst.data());

    // -----------------------------------------------------------------
    // Step 5: 全图 Inverse VST 变换
    // -----------------------------------------------------------------
    vst_inverse_anscombe(vst_full_dst.data(), N_full, dst, nm);
}
// ------------------- 安全去噪 -------------------
int Run_RawNR(stISPParams* gISPparam, u16* src, u16* dst)
{
    int rows = gISPparam->rawinfo.H;
    int cols = gISPparam->rawinfo.W;
    unsigned hw = std::thread::hardware_concurrency();

    // 2. 构造 VST 噪声模型 (a: 散粒噪声增益, b: 读出噪声方差)
    // 💡 提示：如果你的 gISPparam 结构体里有 Sensor 校准的 noise_a / noise_b，请直接赋值；
    //    如果没有，可以先填入推荐的默认基准值 (例如 a = 0.005f, b = 1.0f) 进行测试
    NoiseModel nm;
    nm.a = 0.005; // 或者如: 0.005f
    nm.b = 1; // 或者如: 1.0f

    // 3. 转换 NLM 的 h 参数 (VST 域下建议范围在 1.0 ~ 2.5 之间)
    // 如果 gISPparam 里的 hr/hg/hb 是 255/1024 量纲的整数，建议适当缩放，或者转换为 float
    float h_r_vst = static_cast<float>(gISPparam->rnr_Param.h_r);
    float h_g_vst = static_cast<float>(gISPparam->rnr_Param.h_g);
    float h_b_vst = static_cast<float>(gISPparam->rnr_Param.h_b);
    float nr_alpha = static_cast<float>(gISPparam->rnr_Param.Nr_alpha);
    // 4. 设置新增的高频死区 (Coring) 与 强边缘透传门限 (Edge Threshold)
    // 💡 如果 gISPparam 中已有对应字段可直接读取，否则填入 VST 域的经典推荐初值：
    float coring_thresh_vst = 1.0f; // 高频死区阈值 (建议 0.5 ~ 1.5)
    float edge_thresh_vst = 3.0f; // 强边缘防外扩门限 (建议 2.0 ~ 5.0，越小越能防黑白外扩)

    // 5. 调用全新的 VST 流水线接口
    denoise_bayer_vst_twoband_pipeline(
        src, dst, rows, cols,
        nm, static_cast<int>(hw),
        h_r_vst, h_g_vst, h_b_vst,
        coring_thresh_vst, edge_thresh_vst, nr_alpha
    );

    return 0;
}
