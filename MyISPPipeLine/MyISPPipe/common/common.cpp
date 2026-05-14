#include<iostream>
#include "common.h"
#include <vector>
#define IDX(x, y, w) ((y) * (w) + (x))
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include"stb_image_write.h"


void errorinfo(int ret, const char* s)
{
	if (ret != -1)
	{
		std::printf("%s\n", s);
	}
}
#if 0
void debug_write16bit_image(u16 H, u16 W, u16* rgbdata,const char* s)
{
    cv::Mat result(H, W, CV_8UC3);
    u32 imgSize = H * W;
    for (int i = 0; i < H; i++)
    {
        for (int j = 0; j < W; j++)
        {
            result.ptr<uchar>(i)[3 * j + 2] = rgbdata[i * W + j + 2 * imgSize] >> 8;//r
            result.ptr<uchar>(i)[3 * j + 1] = rgbdata[i * W + j] >> 8;//g
            result.ptr<uchar>(i)[3 * j] = rgbdata[i * W + j + 1 * imgSize] >> 8;//b
            //printf("r=%d,g=%d,b=%d\n", result.ptr<uchar>(i)[3 * j + 2], result.ptr<uchar>(i)[3 * j + 1], result.ptr<uchar>(i)[3 * j]);
        }
    }
    cv::imwrite(s, result);
}
void debug_write12bit_image(u16 H, u16 W, u16* rgbdata, const char* s)
{
    cv::Mat result(H, W, CV_8UC3);
    u32 imgSize = H * W;
    for (int i = 0; i < H; i++)
    {
        for (int j = 0; j < W; j++)
        {
            result.ptr<uchar>(i)[3 * j + 2] = rgbdata[i * W + j + 2 * imgSize] >> 4;//r
            result.ptr<uchar>(i)[3 * j + 1] = rgbdata[i * W + j] >> 4;//g
            result.ptr<uchar>(i)[3 * j] = rgbdata[i * W + j + 1 * imgSize] >> 4;//b
            //printf("r=%d,g=%d,b=%d\n", result.ptr<uchar>(i)[3 * j + 2], result.ptr<uchar>(i)[3 * j + 1], result.ptr<uchar>(i)[3 * j]);
        }
    }
    cv::imwrite(s, result);
}
void debug_write10bit_image(u16 H, u16 W, u16* rgbdata, const char* s)
{
    cv::Mat result(H, W, CV_8UC3);
    u32 imgSize = H * W;
    for (int i = 0; i < H; i++)
    {
        for (int j = 0; j < W; j++)
        {
            result.ptr<uchar>(i)[3 * j + 2] = rgbdata[i * W + j + 2 * imgSize] >> 2;//r
            result.ptr<uchar>(i)[3 * j + 1] = rgbdata[i * W + j] >> 2;//g
            result.ptr<uchar>(i)[3 * j] = rgbdata[i * W + j + 1 * imgSize] >> 2;//b
            //printf("r=%d,g=%d,b=%d\n", result.ptr<uchar>(i)[3 * j + 2], result.ptr<uchar>(i)[3 * j + 1], result.ptr<uchar>(i)[3 * j]);
        }
    }
    cv::imwrite(s, result);
}
void debug_write8bit_image(u16 H, u16 W, u8* rgbdata, const char* s)
{
    cv::Mat result(H, W, CV_8UC3);
    u32 imgSize = H * W;
    for (int i = 0; i < H; i++)
    {
        for (int j = 0; j < W; j++)
        {
            result.ptr<uchar>(i)[3 * j + 2] = rgbdata[i * W + j + 2 * imgSize] ;//r
            result.ptr<uchar>(i)[3 * j + 1] = rgbdata[i * W + j] ;//g
            result.ptr<uchar>(i)[3 * j] = rgbdata[i * W + j + 1 * imgSize] ;//b
            //printf("r=%d,g=%d,b=%d\n", result.ptr<uchar>(i)[3 * j + 2], result.ptr<uchar>(i)[3 * j + 1], result.ptr<uchar>(i)[3 * j]);
        }
    }
    cv::imwrite(s, result);
}
#endif
template <typename T>
void internal_stb_write(u16 H, u16 W, T* rgbdata, const char* s, int shift) {
    // 创建一个临时缓存用于存放交织的 RGBRGB... 格式 (stb需要这种格式)
    std::vector<u8> buffer(H * W * 3);
    u32 imgSize = (u32)H * W;

    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            u32 idx = i * W + j;
            u32 bufIdx = (i * W + j) * 3;

            // 原逻辑：rgbdata[idx] 是 G, [+imgSize] 是 B, [+2*imgSize] 是 R
            // stb_write_jpg 需要的顺序是 R, G, B
            buffer[bufIdx + 0] = (u8)(rgbdata[idx + 2 * imgSize] >> shift); // R
            buffer[bufIdx + 1] = (u8)(rgbdata[idx] >> shift);               // G
            buffer[bufIdx + 2] = (u8)(rgbdata[idx + 1 * imgSize] >> shift); // B
        }
    }

    // 保存为 JPG，质量设为 90
    stbi_write_jpg(s, W, H, 3, buffer.data(), 90);
}

// --- 以下是你的接口实现 ---

void debug_write16bit_image(u16 H, u16 W, u16* rgbdata, const char* s) {
    internal_stb_write(H, W, rgbdata, s, 8);
}

void debug_write12bit_image(u16 H, u16 W, u16* rgbdata, const char* s) {
    internal_stb_write(H, W, rgbdata, s, 4);
}

void debug_write10bit_image(u16 H, u16 W, u16* rgbdata, const char* s) {
    internal_stb_write(H, W, rgbdata, s, 2);
}

void debug_write8bit_image(u16 H, u16 W, u8* rgbdata, const char* s) {
    // 8bit 无需位移
    internal_stb_write(H, W, rgbdata, s, 0);
}
void box_filter_integral(double* img, double* result, int W, int H, int r)
{
    int win_size = 2 * r + 1;
    double norm_factor = 1.0 / (win_size * win_size);

    double* integral = (double*)malloc(sizeof(double) * (W + 1) * (H + 1));
    memset(integral, 0, sizeof(double) * (W + 1) * (H + 1));

    for (int y = 1; y <= H; y++) {
        double row_sum = 0;
        for (int x = 1; x <= W; x++) {
            row_sum += img[(y - 1) * W + (x - 1)];
            integral[y * (W + 1) + x] = integral[(y - 1) * (W + 1) + x] + row_sum;
        }
    }

#pragma omp parallel for
    for (int y = 0; y < H; y++) {
        int y0 = (y - r < 0) ? 0 : y - r;
        int y1 = (y + r >= H) ? H - 1 : y + r;
        for (int x = 0; x < W; x++) {
            int x0 = (x - r < 0) ? 0 : x - r;
            int x1 = (x + r >= W) ? W - 1 : x + r;

            double sum = integral[(y1 + 1) * (W + 1) + (x1 + 1)]
                - integral[y0 * (W + 1) + (x1 + 1)]
                - integral[(y1 + 1) * (W + 1) + x0]
                + integral[y0 * (W + 1) + x0];
            result[y * W + x] = sum * norm_factor;
        }
    }

    free(integral);
}

void GuidedFilter(double* rawdataLog, double* rawdataLogBase, int W, int H, int r, double eps)
{
    int N = W * H;

    double* mean_I = (double*)malloc(sizeof(double) * N);
    double* mean_p = (double*)malloc(sizeof(double) * N);
    double* II = (double*)malloc(sizeof(double) * N);
    double* Ip = (double*)malloc(sizeof(double) * N);


#pragma omp parallel for
    for (int i = 0; i < N; i++) {
        rawdataLogBase[i] = rawdataLog[i];
        // rawdataLogBase初始化可先拷贝 rawdataLog
    }

    // 计算 I*I 和 I*p
#pragma omp parallel for
    for (int i = 0; i < N; i++) {
        II[i] = rawdataLog[i] * rawdataLog[i];
        Ip[i] = rawdataLog[i] * rawdataLogBase[i]; // rawdataLogBase初始化可先拷贝 rawdataLog
    }

    // box_filter计算均值
    box_filter_integral(rawdataLog, mean_I, W, H, r);
    box_filter_integral(rawdataLogBase, mean_p, W, H, r);
    box_filter_integral(II, II, W, H, r);       // mean_II 复用 II
    box_filter_integral(Ip, Ip, W, H, r);       // mean_Ip 复用 Ip

    // 计算线性系数 a, b
    double* a = (double*)malloc(sizeof(double) * N);
    double* b = (double*)malloc(sizeof(double) * N);

#pragma omp parallel for
    for (int i = 0; i < N; i++) {
        a[i] = (Ip[i] - mean_I[i] * mean_p[i]) / (II[i] - mean_I[i] * mean_I[i] + eps);
        b[i] = mean_p[i] - a[i] * mean_I[i];
    }

    // 对 a 和 b 做均值滤波
    double* mean_a = (double*)malloc(sizeof(double) * N);
    double* mean_b = (double*)malloc(sizeof(double) * N);
    box_filter_integral(a, mean_a, W, H, r);
    box_filter_integral(b, mean_b, W, H, r);

    // 输出 q
#pragma omp parallel for
    for (int i = 0; i < N; i++)
        rawdataLogBase[i] = mean_a[i] * rawdataLog[i] + mean_b[i];

    free(mean_I);
    free(mean_p);
    free(II);
    free(Ip);
    free(a);
    free(b);
    free(mean_a);
    free(mean_b);
}
// ---------------- Guided Filter 高频 ----------------
 void guidedFilterHighFreq_float(const float* hf, const float* guide, float* out,
    int rows, int cols, int r, float eps)
{
    int size_window = (2 * r + 1) * (2 * r + 1);
    float* mean_hf = (float*)malloc(sizeof(float) * rows * cols);
    float* mean_g = (float*)malloc(sizeof(float) * rows * cols);
    float* corr_hfg = (float*)malloc(sizeof(float) * rows * cols);
    float* corr_gg = (float*)malloc(sizeof(float) * rows * cols);

    // 局部均值
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            float sum_hf = 0.0f, sum_g = 0.0f, sum_hfg = 0.0f, sum_gg = 0.0f;
            int cnt = 0;
            for (int dy = -r; dy <= r; dy++) {
                int yy = y + dy; if (yy < 0)yy = 0; if (yy >= rows)yy = rows - 1;
                for (int dx = -r; dx <= r; dx++) {
                    int xx = x + dx; if (xx < 0)xx = 0; if (xx >= cols)xx = cols - 1;
                    float g = guide[IDX(xx, yy, cols)];
                    float h = hf[IDX(xx, yy, cols)];
                    sum_hf += h; sum_g += g; sum_hfg += h * g; sum_gg += g * g;
                    cnt++;
                }
            }
            mean_hf[IDX(x, y, cols)] = sum_hf / cnt;
            mean_g[IDX(x, y, cols)] = sum_g / cnt;
            corr_hfg[IDX(x, y, cols)] = sum_hfg / cnt;
            corr_gg[IDX(x, y, cols)] = sum_gg / cnt;
        }
    }

    // a,b
    float* a = (float*)malloc(sizeof(float) * rows * cols);
    float* b = (float*)malloc(sizeof(float) * rows * cols);
    for (int i = 0; i < rows * cols; i++) {
        float var = corr_gg[i] - mean_g[i] * mean_g[i];
        float cov = corr_hfg[i] - mean_hf[i] * mean_g[i];
        a[i] = cov / (var + eps);
        b[i] = mean_hf[i] - a[i] * mean_g[i];
    }

    // 平滑 a,b
    float* mean_a = (float*)malloc(sizeof(float) * rows * cols);
    float* mean_b = (float*)malloc(sizeof(float) * rows * cols);
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            float sum_a = 0.0f, sum_b = 0.0f;
            int cnt = 0;
            for (int dy = -r; dy <= r; dy++) {
                int yy = y + dy; if (yy < 0)yy = 0; if (yy >= rows)yy = rows - 1;
                for (int dx = -r; dx <= r; dx++) {
                    int xx = x + dx; if (xx < 0)xx = 0; if (xx >= cols)xx = cols - 1;
                    sum_a += a[IDX(xx, yy, cols)];
                    sum_b += b[IDX(xx, yy, cols)];
                    cnt++;
                }
            }
            mean_a[IDX(x, y, cols)] = sum_a / cnt;
            mean_b[IDX(x, y, cols)] = sum_b / cnt;
        }
    }

    // 输出
    for (int i = 0; i < rows * cols; i++)
        out[i] = mean_a[i] * guide[i] + mean_b[i];

    free(mean_hf); free(mean_g); free(corr_hfg); free(corr_gg);
    free(a); free(b); free(mean_a); free(mean_b);
}
