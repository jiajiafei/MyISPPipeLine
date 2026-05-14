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
void downsample2x(const float* src, int w, int h, float* dst) {
    int w2 = w / 2, h2 = h / 2;
    for (int y = 0; y < h2; y++) {
        for (int x = 0; x < w2; x++) {
            float sum = src[idx(2 * x, 2 * y, w)] + src[idx(2 * x + 1, 2 * y, w)]
                + src[idx(2 * x, 2 * y + 1, w)] + src[idx(2 * x + 1, 2 * y + 1, w)];
            dst[idx(x, y, w2)] = sum * 0.25f;
        }
    }
}
void upsample2x(const float* src, int w, int h, float* dst) {
    int w2 = w * 2, h2 = h * 2;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float val = src[idx(x, y, w)];
            dst[idx(2 * x, 2 * y, w2)] = val; dst[idx(2 * x + 1, 2 * y, w2)] = val;
            dst[idx(2 * x, 2 * y + 1, w2)] = val; dst[idx(2 * x + 1, 2 * y + 1, w2)] = val;
        }
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
void computeIntegralImagesFloat(const float* src, int w, int h,
    std::vector<double>& integral, std::vector<double>& integralSq) {
    integral.assign((w + 1) * (h + 1), 0.0);
    integralSq.assign((w + 1) * (h + 1), 0.0);
    for (int y = 1; y <= h; ++y) {
        double sumRow = 0.0, sumRowSq = 0.0;
        for (int x = 1; x <= w; ++x) {
            float val = src[idx(x - 1, y - 1, w)];
            sumRow += val;
            sumRowSq += double(val) * double(val);
            int id = y * (w + 1) + x;
            integral[id] = integral[(y - 1) * (w + 1) + x] + sumRow;
            integralSq[id] = integralSq[(y - 1) * (w + 1) + x] + sumRowSq;
        }
    }
}
inline double boxSum(const std::vector<double>& integral, int W1, int x1, int y1, int x2, int y2) {
    x1 = max(0, min(x1, W1 - 1));
    x2 = max(0, min(x2, W1 - 1));
    int H1 = (int)(integral.size() / W1);
    y1 = max(0, min(y1, H1 - 1));
    y2 = max(0, min(y2, H1 - 1));
    return integral[y2 * W1 + x2] - integral[y1 * W1 + x2] - integral[y2 * W1 + x1] + integral[y1 * W1 + x1];
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

// ---------------- NLM with integral prefilter ----------------
// src: float image (VST domain), dst: output
// use integral to precompute mean/var for blocks to prefilter unlikely candidates
// 替换版 nlm_channel：使用 per-shift 差平方积分图加速 SSD 计算
void nlm_channel(const float* src, float* dst, int w, int h,
    int patchRadiusX, int patchRadiusY,
    int searchRadius, float h_param, int numThreads)
{
    int padX = patchRadiusX, padY = patchRadiusY;
    int Npix = w * h;
    double h2 = h_param * h_param;
    struct Patch {
        int x1, y1, x2, y2;
        double mean, var, N;
    };
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

    int tileH = max(1, h / numThreads);
    std::vector<std::thread> ths;

    for (int t = 0; t < numThreads; ++t) {
        int y0 = t * tileH;
        int y1 = (t == numThreads - 1) ? h : y0 + tileH;

        ths.emplace_back([=, &src, &dst, &patchInfo, &integral, &integralSq]() {
            std::vector<double> weightSum_local(Npix, 0.0);
            std::vector<double> pixelSum_local(Npix, 0.0);
            std::vector<double> integralDiff((w + 1) * (h + 1), 0.0);

            for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
                for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
                    std::fill(integralDiff.begin(), integralDiff.end(), 0.0);
                    for (int y = 0; y < h; ++y) {
                        double rowSum = 0.0;
                        for (int x = 0; x < w; ++x) {
                            int qx = x + dx;
                            int qy = y + dy;
                            double d2 = 0.0;
                            if (qx >= 0 && qx < w && qy >= 0 && qy < h) {
                                int qx_safe = min(max(qx, 0), w - 1);
                                int qy_safe = min(max(qy, 0), h - 1);
                                double diff = src[idx(x, y, w)] - src[idx(qx_safe, qy_safe, w)];

                                d2 = diff * diff;
                            }
                            rowSum += d2;
                            int id = (y + 1) * (w + 1) + (x + 1);
                            integralDiff[id] = integralDiff[id - (w + 1)] + rowSum;
                        }
                    }

                    for (int y = y0; y < y1; ++y) {
                        for (int x = 0; x < w; ++x) {
                            int idx0 = idx(x, y, w);
                            Patch& refPatch = patchInfo[idx0];

                            int sx = x + dx;
                            int sy = y + dy;
                            if (sx < 0 || sx >= w || sy < 0 || sy >= h) continue;

                            int nx1 = max(0, sx - padX);
                            int ny1 = max(0, sy - padY);
                            int nx2 = min(w - 1, sx + padX);
                            int ny2 = min(h - 1, sy + padY);
                            double sum = boxSum(integral, W1, nx1, ny1, nx2, ny2);
                            double sumSq = boxSum(integralSq, W1, nx1, ny1, nx2, ny2);
                            double N = double((nx2 - nx1 + 1) * (ny2 - ny1 + 1));
                            double mean = sum / N;
                            double var = sumSq / N - mean * mean;
                            if (var < 0) var = 0;

                            double meanDiff = refPatch.mean - mean;
                            if (meanDiff * meanDiff > 50.0 + refPatch.var + var) continue;

                            double ssd = boxSum(integralDiff, W1, refPatch.x1, refPatch.y1, refPatch.x2, refPatch.y2);
                            double dist2 = ssd / refPatch.N;
                            double wgt = exp(-dist2 / h2);

                            // 安全访问
                            int sx_safe = min(max(sx, 0), w - 1);
                            int sy_safe = min(max(sy, 0), h - 1);
                            pixelSum_local[idx0] += wgt * src[idx(sx_safe, sy_safe, w)];
                            weightSum_local[idx0] += wgt;
                        }
                    }
                }
            }

            for (int y = y0; y < y1; ++y) {
                for (int x = 0; x < w; ++x) {
                    int idx0 = idx(x, y, w);
                    dst[idx0] = (weightSum_local[idx0] > 0.0) ? float(pixelSum_local[idx0] / weightSum_local[idx0]) : src[idx0];
                }
            }
            });
    }

    for (auto& th : ths) th.join();
}
// 替换你的 nlm_channel_fast 为下面实现
void nlm_channel_fast(const float* src, float* dst, int w, int h,
    int patchRadiusX, int patchRadiusY,
    int searchRadius, float h_param, int numThreads)
{
    const int step = 2; // 可调：1 = 精确（慢），2 = 4x 加速，4 = 16x 加速（但失真更大）
    int padX = patchRadiusX, padY = patchRadiusY;
    int Npix = w * h;
    double h2 = double(h_param) * double(h_param);

    struct Patch { int x1, y1, x2, y2; double mean, var, N; };
    std::vector<double> integral, integralSq;
    computeIntegralImagesFloat(src, w, h, integral, integralSq);
    int W1 = w + 1;

    // 预计算每个像素的 patch info（用于快速 mean/var 预筛选）
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

    // 全局稀疏累加数组（线程按 tile 写入，无冲突）
    std::vector<double> weightSum(Npix, 0.0);
    std::vector<double> pixelSum(Npix, 0.0);

    int tileH = max(1, h / numThreads);
    std::vector<std::thread> ths;

    for (int t = 0; t < numThreads; ++t) {
        int y0 = t * tileH;
        int y1 = (t == numThreads - 1) ? h : (y0 + tileH);

        ths.emplace_back([=, &src, &patchInfo, &integral, &integralSq, &weightSum, &pixelSum]() {
            // 每个线程复用的临时 integralDiff（全分辨率）
            std::vector<double> integralDiff((w + 1) * (h + 1), 0.0);

            // 对所有偏移构建差分积分图，但只对稀疏网格计算权重
            for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
                for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
                    // build integralDiff at full resolution (needed for correct boxSum)
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

                    // 在本线程负责的 tile 区域上，按 step 计算稀疏网格的权重并累加到全局数组
                    for (int yy = y0; yy < y1; yy += step) {
                        for (int xx = 0; xx < w; xx += step) {
                            int idx0 = idx(xx, yy, w);
                            const Patch& refPatch = patchInfo[idx0];

                            int sx = xx + dx, sy = yy + dy;
                            if (sx < 0 || sx >= w || sy < 0 || sy >= h) continue;

                            // neighbor patch mean/var via integral
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
                            if (meanDiff * meanDiff > 50.0 + refPatch.var + varN) continue; // prefilter

                            // SSD via integralDiff (O(1))
                            double ssd = boxSum(integralDiff, W1, refPatch.x1, refPatch.y1, refPatch.x2, refPatch.y2);
                            double dist2 = ssd / refPatch.N;
                            double wgt = exp(-dist2 / h2);

                            // 安全取中心像素值作为聚合项
                            double neiVal = double(src[idx(sx, sy, w)]);

                            // 因为每个线程只负责写入 y ∈ [y0,y1) 的索引（间隔 step），不会和其他线程冲突
                            weightSum[idx0] += wgt;
                            pixelSum[idx0] += wgt * neiVal;
                        }
                    }
                }
            }
            });
    }

    // 等待所有线程完成
    for (auto& th : ths) if (th.joinable()) th.join();

    // 生成稀疏输出值 (只在 step 网格上有值)
    std::vector<float> sparseVal(Npix, 0.0f);
    std::vector<unsigned char> computed(Npix, 0); // 0/1 标记哪些位置有计算结果
    for (int y = 0; y < h; y += step) {
        for (int x = 0; x < w; x += step) {
            int id = idx(x, y, w);
            if (weightSum[id] > 0.0) {
                sparseVal[id] = float(pixelSum[id] / weightSum[id]);
                computed[id] = 1;
            }
            else {
                // 若没有任何邻居贡献，则退回到原始像素（或可以用局部均值）
                sparseVal[id] = src[id];
                computed[id] = 1;
            }
        }
    }

    // 双线性插值恢复稠密图（对每个像素），并行化
#pragma omp parallel for schedule(static) num_threads(numThreads)
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int gx0 = (x / step) * step;
            int gy0 = (y / step) * step;
            int gx1 = min(gx0 + step, w - 1);
            int gy1 = min(gy0 + step, h - 1);

            float v00 = sparseVal[idx(gx0, gy0, w)];
            float v10 = sparseVal[idx(gx1, gy0, w)];
            float v01 = sparseVal[idx(gx0, gy1, w)];
            float v11 = sparseVal[idx(gx1, gy1, w)];

            float wx = (gx1 == gx0) ? 0.0f : float(x - gx0) / float(gx1 - gx0);
            float wy = (gy1 == gy0) ? 0.0f : float(y - gy0) / float(gy1 - gy0);

            // bilinear
            float a = v00 * (1 - wx) + v10 * wx;
            float b = v01 * (1 - wx) + v11 * wx;
            float val = a * (1 - wy) + b * wy;

            dst[idx(x, y, w)] = val;
        }
    }
}

void downsample2x_gauss(const float* src, int w, int h, float* dst) {
    int w2 = max(1, w / 2), h2 = max(1, h / 2);
    float k[3] = { 0.25f,0.5f,0.25f };
    for (int y = 0; y < h2; y++) {
        for (int x = 0; x < w2; x++) {
            float sum = 0.f;
            for (int ky = -1; ky <= 1; ky++) {
                int yy = min(max(2 * y + ky, 0), h - 1);
                for (int kx = -1; kx <= 1; kx++) {
                    int xx = min(max(2 * x + kx, 0), w - 1);
                    sum += src[idx(xx, yy, w)] * k[kx + 1] * k[ky + 1];
                }
            }
            dst[idx(x, y, w2)] = sum;
        }
    }
}
void nlm_multiscale(const float* src, float* dst, int w, int h, int patchRx, int patchRy, int searchR, float h_param, int numThreads, int levels) {
    if (levels < 1) levels = 1;
    std::vector<std::vector<float>> pyr(levels);
    std::vector<int> ws(levels), hs(levels);
    printf("1\n");
    // build pyramid (pyr[0]=finest)
    ws[0] = w; hs[0] = h;
    pyr[0].assign(src, src + w * h);
    for (int l = 1; l < levels; ++l) {
        int w0 = ws[l - 1], h0 = hs[l - 1];
        int w1 = max(1, w0 / 2), h1 = max(1, h0 / 2);
        ws[l] = w1; hs[l] = h1;
        pyr[l].resize(w1 * h1);
        downsample2x_gauss(pyr[l - 1].data(), w0, h0, pyr[l].data());
    }
 
    // parameters for fusion
    const float h_scale = 1.6f;     // increases h for coarser layers
    const float res_scale = 0.6f;   // residual denoise is lighter
    const float detail_gain = 0.8; // how strongly to add back denoised residual
    const float blend = 0.9;      // blend between reconstruction and original (per-layer)
    const float orig_mix = 0.12f;   // final small mix with original to boost sharpness

    // process bottom (coarsest) layer first
    int lbot = levels - 1;
#if 0
    {
        int wl = ws[lbot], hl = hs[lbot];
        std::vector<float> denoised(wl * hl);
        float h_level = h_param * powf(h_scale, float(lbot)); // coarsest gets largest h
        nlm_channel(pyr[lbot].data(), denoised.data(), wl, hl, patchRx, patchRy, searchR, h_level, numThreads);
        pyr[lbot].swap(denoised); // now pyr[lbot] holds denoised
    }
#endif

    // coarse-to-fine residual denoise and fusion
    for (int l = lbot - 1; l >= 0; --l) {
        int w_cur = ws[l], h_cur = hs[l];
        int w_coarse = ws[l + 1], h_coarse = hs[l + 1];

        // upsample denoised coarse to current size
        std::vector<float> up(w_cur * h_cur);
        upsample2x(pyr[l + 1].data(), w_coarse, h_coarse, up.data());
 
        // compute residual = original_current - up
        std::vector<float> residual(w_cur * h_cur);
        for (int i = 0; i < w_cur * h_cur; ++i) residual[i] = pyr[l][i] - up[i];

        // denoise residual with a lighter (smaller) effective h
        std::vector<float> res_denoised(w_cur * h_cur);
        float h_level = h_param * powf(h_scale, float(l));         // base per-level h
        float h_res = h_level * res_scale;                         // residual uses smaller h
        nlm_channel_fast(residual.data(), res_denoised.data(), w_cur, h_cur, patchRx / 2, patchRy / 2, searchR / 2, h_res, numThreads);


        // reconstruct: up + detail_gain * res_denoised
        std::vector<float> recon(w_cur * h_cur);
        for (int i = 0; i < w_cur * h_cur; ++i) {
            float added = up[i] + detail_gain * res_denoised[i];
            // blend with original to avoid over-smoothing
            recon[i] = blend * added + (1.0f - blend) * pyr[l][i];
        }

        // set current layer denoised to recon
        pyr[l].swap(recon);
    }

    // result at finest level
    int N = w * h;
    // small final mix with original to restore micro-contrast
    for (int i = 0; i < N; ++i) {
        dst[i] = (1.0f - orig_mix) * pyr[0][i] + orig_mix * src[i];
    }

}
void nlm_multiscale_hybrid(
    const float* src, float* dst,
    int w, int h,
    int patchRx, int patchRy,
    int searchR,
    float h_param,
    int numThreads,
    int levels)
{
    if (levels < 1) levels = 1;
    std::vector<std::vector<float>> pyr(levels);
    std::vector<int> ws(levels), hs(levels);

    // 1️⃣ 构建金字塔
    ws[0] = w; hs[0] = h;
    pyr[0].assign(src, src + w * h);
    for (int l = 1; l < levels; ++l) {
        int w0 = ws[l - 1], h0 = hs[l - 1];
        int w1 = max(1, w0 / 2), h1 = max(1, h0 / 2);
        ws[l] = w1; hs[l] = h1;
        pyr[l].resize(w1 * h1);
        downsample2x_gauss(pyr[l - 1].data(), w0, h0, pyr[l].data());
    }

    // 参数调节
    const float h_scale = 12;
    const float res_scale = 1.6f;
    const float detail_gain = 0.5;
    const float blend = 1;
    const float orig_mix = 0.1f;

    int lbot = levels - 1;

    // 2️⃣ 最底层 NLM（最平滑的结构层）
    {
        int wl = ws[lbot], hl = hs[lbot];
        std::vector<float> denoised(wl * hl);
        float h_level = h_param * powf(h_scale, float(lbot));
        nlm_channel_fast(pyr[lbot].data(), denoised.data(),
            wl, hl, patchRx, patchRy, searchR, h_level, numThreads);
        pyr[lbot].swap(denoised);
    }

    // 3️⃣ 从次底层开始向上融合
    for (int l = lbot - 1; l >= 0; --l) {
        int w_cur = ws[l], h_cur = hs[l];
        int w_coarse = ws[l + 1], h_coarse = hs[l + 1];

        std::vector<float> up(w_cur * h_cur);
        upsample2x(pyr[l + 1].data(), w_coarse, h_coarse, up.data());

        std::vector<float> residual(w_cur * h_cur);
        for (int i = 0; i < w_cur * h_cur; ++i)
            residual[i] = pyr[l][i] - up[i];

        std::vector<float> res_denoised(w_cur * h_cur);
        float h_level = h_param * powf(h_scale, float(l));
        float h_res = h_level * res_scale;

        // ✅ 仅次底层再做一次 NLM，其他层直接跳过
        if (l == lbot - 1) {
            nlm_channel_fast(residual.data(), res_denoised.data(),
                w_cur, h_cur, patchRx / 2, patchRy / 2, searchR / 2,
                h_res, numThreads);
        }
        else {
            res_denoised = residual;  // 跳过NLM
        }

        // 重建
        std::vector<float> recon(w_cur * h_cur);
        for (int i = 0; i < w_cur * h_cur; ++i) {
            float added = up[i] + detail_gain * res_denoised[i];
            recon[i] = blend * added + (1.0f - blend) * pyr[l][i];
        }

        pyr[l].swap(recon);
    }

    // 4️⃣ 最终融合原图
    int N = w * h;
    for (int i = 0; i < N; ++i)
        dst[i] = (1.0f - orig_mix) * pyr[0][i] + orig_mix * src[i];
}



// ---------------- Noise compensation (suppression + back-projection) ----------------
// input: src_before (original VST), src_after (filtered VST) -> compute k' = before - after
// apply piecewise suppression on k' and add back to filtered.
// alpha schedule: you can make it depend on local variance or absolute magnitude.
void noise_compensate_and_backproject(const float* before, float* after, int n) {
    for (int i = 0; i < n; ++i) {
        float k = before[i] - after[i];
        float abs_k = fabsf(k);
        float alpha;
        if (abs_k < 2.0f) alpha = 0.9f;
        else if (abs_k < 8.0f) alpha = 0.6f;
        else alpha = 0.3f;
        after[i] = after[i] + alpha * k;
    }
}
void process_channel_vst_multiscale(u16* channel16, int w, int h, const NoiseModel& nm, int patchRx, int patchRy, int searchR, float h_param, int numThreads, int pyramidLevels) {
    int N = w * h;
    std::vector<float> vst(N), vst_out(N);
    vst_forward_anscombe(channel16, N, vst.data(), nm);
    nlm_multiscale_hybrid(vst.data(), vst_out.data(), w, h, patchRx, patchRy, searchR, h_param, numThreads, pyramidLevels);
    noise_compensate_and_backproject(vst.data(), vst_out.data(), N);
    vst_inverse_anscombe(vst_out.data(), N, channel16, nm);
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

// ---------------- Top-level function integrating everything ----------------
void denoise_bayer_vst_nlm_pipeline(u16* raw, u16* dst, int rows, int cols, int numThreadsAll, int hr, int hg, int hb) {
    int h2 = rows / 2, w2 = cols / 2;
    int N = w2 * h2;
    std::vector<u16> R(N), Gr(N), Gb(N), B(N);
    splitBayer(raw, rows, cols, R.data(), Gr.data(), Gb.data(), B.data());
    int pyramidLevels = 3;
    // fit / load noise models per channel (replace stub with real calibration)
    NoiseModel nmR = fitNoiseModel_stub();
    NoiseModel nmGr = fitNoiseModel_stub();
    NoiseModel nmGb = fitNoiseModel_stub();
    NoiseModel nmB = fitNoiseModel_stub();

    unsigned hw = std::thread::hardware_concurrency();
    int perChanThreads = max(1u, hw / 1);
    if (perChanThreads > numThreadsAll) perChanThreads = numThreadsAll;

    // channel lambda
    auto runChan = [&](u16* ch, const NoiseModel& nm, bool useDiamond, int patchRx, int patchRy, int searchR, float hparam) {
        process_channel_vst_multiscale(ch, w2, h2, nm, patchRx, patchRy, searchR, hparam, perChanThreads, pyramidLevels);
    };
    float h_r = hr;
    float h_g = hg;
    float h_b = hb;

    // spawn per-channel threads to run in parallel (if you have enough cores)
    std::thread tR(runChan, R.data(), nmR, false, 2, 2, 6, h_r);    // R: rect patch 5x5 -> rx=2 ry=2
    std::thread tGr(runChan, Gr.data(), nmGr, false, 2, 2, 6, h_g);   // Gr: diamond
    std::thread tGb(runChan, Gb.data(), nmGb, false, 2, 2, 6, h_g);   // Gb: diamond
    std::thread tB(runChan, B.data(), nmB, false, 2, 2, 6, h_b);   // B: rect

    tR.join(); tGr.join(); tGb.join(); tB.join();

    // merge back
    mergeBayer(raw, rows, cols, R.data(), Gr.data(), Gb.data(), B.data());
}


// ------------------- 安全去噪 -------------------
int Run_RawNR(stISPParams* gISPparam, u16* src, u16* dst)
{
    int hr = gISPparam->rnr_Param.h_r;
    int hg = gISPparam->rnr_Param.h_g;
    int hb = gISPparam->rnr_Param.h_b;
    int rows = gISPparam->rawinfo.H;
    int cols = gISPparam->rawinfo.W;


    // get hardware concurrency and choose thread counts
    unsigned hw = std::thread::hardware_concurrency();

    denoise_bayer_vst_nlm_pipeline(src, dst, rows, cols, int(hw), hr, hg, hb);
    return 0;
}
