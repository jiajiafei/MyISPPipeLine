#include"Tonemap.h"
#include<stdlib.h>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include<vector>
#include <variant>
#include <time.h>
#include <omp.h>

#define LOG10E 0.4342944819032518  // 1 / ln(10)

inline double fast_log10(double x) {
    return log(x) * LOG10E;
}

inline double fast_pow10(double x) {
    return exp(x / LOG10E); // M_LN10 = ln(10)
}


using namespace std;

void convertToY(int* rawdata, int* rawdata_y, int H, int W)
{
    for (int row = 0; row < H / 2; row++)
    {
        for (int col = 0; col < W / 2; col++)
        {
            int tempR = rawdata[row * 2 * W + col * 2];
            int tempB = rawdata[(row * 2 + 1) * W + col * 2 + 1];
            int tempG = (rawdata[row * 2 * W + col * 2 + 1] + rawdata[(row * 2 + 1) * W + col * 2]) / 2;
            // rawdata_y[row * W / 2 + col] = (20 * tempR + 40 * tempG + 1 * tempB) / 61;
            rawdata_y[row * W / 2 + col] = (0.25 * tempR + 0.5 * tempG + 0.25 * tempB);
        }
    }
}
void convertTomaxbayer(int* rawdata, int* rawdata_y, int H, int W, eBayerPattern bayerpattern)
{
    int tempR = 0;
    int tempB = 0;
    int tempG = 0;

    for (int row = 0; row < H / 2; row++)
    {
        for (int col = 0; col < W / 2; col++)
        {
            switch (bayerpattern)
            {
            case RGGB:
                tempR = rawdata[row * 2 * W + col * 2];
                tempB = rawdata[(row * 2 + 1) * W + col * 2 + 1];
                tempG = (rawdata[row * 2 * W + col * 2 + 1] + rawdata[(row * 2 + 1) * W + col * 2]) / 2;
                 break;
            case BGGR:
                tempB = rawdata[row * 2 * W + col * 2];
                tempR = rawdata[(row * 2 + 1) * W + col * 2 + 1];
                tempG = (rawdata[row * 2 * W + col * 2 + 1] + rawdata[(row * 2 + 1) * W + col * 2]) / 2;
                break;
            case GBRG:
                tempR = rawdata[(row * 2 + 1) * W + col * 2];
                tempB = rawdata[row * 2 * W + col * 2 + 1];
                tempG = (rawdata[row * 2 * W + col * 2] + rawdata[(row * 2 + 1) * W + col * 2 + 1]) / 2;
                break;
            case GRBG:
                tempR = rawdata[(row * 2 + 1) * W + col * 2];
                tempB = rawdata[row * 2 * W + col * 2 + 1];
                tempG = (rawdata[row * 2 * W + col * 2] + rawdata[(row * 2 + 1) * W + col * 2 + 1]) / 2;
                break;
            case CRGC:
                tempR = rawdata[(row * 2 + 1) * W + col * 2];
                tempB = rawdata[row * 2 * W + col * 2 + 1];
                tempG = (rawdata[row * 2 * W + col * 2] + rawdata[(row * 2 + 1) * W + col * 2 + 1]) / 2;
                break;
            default:
                printf("This pattern is illegal!\n");
            }
             
            rawdata_y[row * W / 2 + col] = max(max(tempR, tempB), tempG);//rggb
        }
    }
}
void convertToMaxrgb(int* rawdata, int* rawdata_y, int H, int W)
{

    int* tempdata = (int*)malloc(sizeof(int) * H * W / 4);
    for (int row = 0; row < H / 2; row++)
    {
        for (int col = 0; col < W / 2; col++)
        {
            int tempR = rawdata[row * 2 * W + col * 2];
            int tempB = rawdata[(row * 2 + 1) * W + col * 2 + 1];
            int tempG = (rawdata[row * 2 * W + col * 2 + 1] + rawdata[(row * 2 + 1) * W + col * 2]) / 2;
            tempdata[row * W / 2 + col] = (0.25 * tempR + 0.5 * tempG + 0.25 * tempB);
            // rawdata_y[row * W / 2 + col] = max(max(tempR,tempB),tempG);
        }
    }

    for (int row = 0; row < H / 2 - 1; row++)
    {
        for (int col = 0; col < W / 2 - 1; col++)
        {
            int temp1 = tempdata[row * W / 2 + col];
            int temp2 = tempdata[(row + 1) * W / 2 + col];
            int temp3 = tempdata[row * W / 2 + col + 1];
            int temp4 = tempdata[(row + 1) * W / 2 + col + 1];

            rawdata_y[row * W / 2 + col] = max(max(temp1, temp2), max(temp3, temp4));

        }
    }




}
void GetYdata(int* rawdata, int* rawdata_y, int H, int W)
{
    int luma_th = 10;
    int luma_slop = 256;//max :1024


    int* rawdata_luma = (int*)malloc(H * W * sizeof(int) / 4);
    int* rawdata_maxrgb = (int*)malloc(H * W * sizeof(int) / 4);

    convertToMaxrgb(rawdata, rawdata_maxrgb, H, W);
    convertToY(rawdata, rawdata_luma, H, W);
    for (int i = 0; i < H / 2; i++)
    {
        for (int j = 0; j < W / 2; j++)
        {

            if (rawdata_luma[i * W / 2 + j] <= luma_th)
            {
                rawdata_y[i * W / 2 + j] = rawdata_maxrgb[i * W / 2 + j];
            }
            else if (rawdata_luma[i * W / 2 + j] >= luma_slop)
            {
                rawdata_y[i * W / 2 + j] = rawdata_luma[i * W / 2 + j];
            }
            else
            {
                float blend = ((float)(rawdata_luma[i * W / 2 + j] - luma_th)) / ((float)(luma_slop - luma_th));
                rawdata_y[i * W / 2 + j] = blend * rawdata_luma[i * W / 2 + j] + (1 - blend) * rawdata_maxrgb[i * W / 2 + j];
            }

        }
    }


}
// 边界扩展（pad = 半径r）
// 模式：BORDER_REPLICATE（复制边缘像素）
void extrnBoard(double* img, double* img_extraboard, int pad_w, int pad_h, int W, int H)
{
    int extra_W = W + 2 * pad_w;
    int extra_H = H + 2 * pad_h;

    // 填中心
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            img_extraboard[(i + pad_h) * extra_W + (j + pad_w)] = img[i * W + j];
        }
    }

    // 上下边界
    for (int i = 0; i < pad_h; i++) {
        // 上边
        for (int j = 0; j < W; j++) {
            img_extraboard[i * extra_W + (j + pad_w)] = img[0 * W + j];
        }
        // 下边
        for (int j = 0; j < W; j++) {
            img_extraboard[(i + H + pad_h) * extra_W + (j + pad_w)] = img[(H - 1) * W + j];
        }
    }

    // 左右边界（包括扩展后的上下边）
    for (int i = 0; i < extra_H; i++) {
        // 左边
        for (int j = 0; j < pad_w; j++) {
            img_extraboard[i * extra_W + j] = img_extraboard[i * extra_W + pad_w];
        }
        // 右边
        for (int j = 0; j < pad_w; j++) {
            img_extraboard[i * extra_W + (j + W + pad_w)] = img_extraboard[i * extra_W + (W + pad_w - 1)];
        }
    }
}
#if 0
//原始
unsigned short* ELTM(int* rawdata_y, stISPParams* gISPparam)
{
    double lamda = gISPparam->tonemap_Param.lamda;//色彩恢复
    int PWLsize = gISPparam->tonemap_Param.PWLsize;
    double tauR = gISPparam->tonemap_Param.tauR;
    double P = gISPparam->tonemap_Param.P;
    double etaF = gISPparam->tonemap_Param.etaF;
    double etaC = gISPparam->tonemap_Param.etaC;
    double BPCmax = gISPparam->tonemap_Param.BPCmax;
    double BPCmin = gISPparam->tonemap_Param.BPCmin;
    double noise_sigma = gISPparam->tonemap_Param.noise_sigma;
    int W=gISPparam->rawinfo.W/2;
    int H = gISPparam->rawinfo.H/2;
    int N = H * W;
    unsigned short* result = (unsigned short*)malloc(sizeof(unsigned short) * N);
    double* logimage = (double*)malloc(sizeof(double) * N);
    double* DPFlog = (double*)malloc(sizeof(double) * N);
    double* DPClog = (double*)malloc(sizeof(double) * N);
    double* BPFlog = (double*)malloc(sizeof(double) * N);
    double* BPlog = (double*)malloc(sizeof(double) * N);

    double maxBPlog = -1e9, minBPlog = 1e9;

    // ---------- Step 1: log transform ----------
#pragma omp parallel for reduction(max:maxBPlog) reduction(min:minBPlog)
    for (int i = 0; i < N; i++) {
        if (rawdata_y[i] == 0)
            logimage[i] = 0.0;
        else
            logimage[i] = fast_log10(rawdata_y[i]);

        if (logimage[i] > maxBPlog) maxBPlog = logimage[i];
        if (logimage[i] < minBPlog) minBPlog = logimage[i];
    }

    // ---------- Step 2: Guided filtering ----------
    GuidedFilter(logimage, BPFlog, W, H, 1, 0.02);

#pragma omp parallel for
    for (int i = 0; i < N; i++) {
        DPFlog[i] = logimage[i] - BPFlog[i];
    }

    GuidedFilter(BPFlog, BPlog, W, H, 2, 1);

#pragma omp parallel for
    for (int i = 0; i < N; i++) {
        DPClog[i] = BPFlog[i] - BPlog[i];
    }

    // ---------- Step 3: Normalize BPlog ----------
    double beta = -maxBPlog;
    double alpha = tauR / (maxBPlog - minBPlog);

#pragma omp parallel for
    for (int i = 0; i < N; i++) {
        BPlog[i] = (BPlog[i] + beta) * alpha;
    }

    minBPlog = (minBPlog + beta) * alpha;
    maxBPlog = (maxBPlog + beta) * alpha;

    double BPmin = fast_pow10(minBPlog);
    double BPmax = fast_pow10(maxBPlog);

    // Precompute constants
    double logP = fast_log10(P);
    double log1pP = fast_log10(1.0 + P);
    double denom = log1pP - logP;

    // ---------- Step 4: Final loop ----------
#pragma omp parallel for
    for (int i = 0; i < N; i++) {
        double SG = (-0.4 * BPlog[i] > 1.0) ? (-0.4 * BPlog[i]) : 1.0;

        double DPFlog_ = etaF * SG * DPFlog[i];
        double DPClog_ = etaC * SG * DPClog[i];

        double BP = fast_pow10(BPlog[i]);
        double DP = fast_pow10(DPFlog_ + DPClog_);

        double tmp = (BP - BPmin) / (BPmax - BPmin) + P;
        double BPC = (BPCmax - BPCmin) * (fast_log10(tmp) - logP) / denom + BPCmin;

        double YC = BPC * DP;

        result[i] = (unsigned short)(YC * maxsize > maxsize ? maxsize : (YC * maxsize < 0 ? 0 : YC * maxsize));
    }
 
    std::free(logimage);
    std::free(DPFlog);
    std::free(DPClog);
    std::free(BPFlog);
    std::free(BPlog);
    return result;
}
#endif
struct HermiteCoeffs {
    double a, b, c, d;
};

// 计算样条系数工具函数
HermiteCoeffs getHermiteCoeffs(double x0, double x1, double y0, double y1, double m0, double m1) {
    double L = x1 - x0;
    if (fabs(L) < 1e-8) L = 1e-8;
    double L2 = L * L;
    double L3 = L2 * L;
    HermiteCoeffs co;
    co.d = y0;
    co.c = m0;
    co.a = (m1 + m0) / L2 - 2.0 * (y1 - y0) / L3;
    co.b = 3.0 * (y1 - y0) / L2 - (m1 + 2.0 * m0) / L;
    return co;
}
unsigned short* ELTM(int* rawdata_y, stISPParams* gISPparam)
{
    // ... 原有参数提取 ...
    double tauR = gISPparam->tonemap_Param.tauR;
    double P = gISPparam->tonemap_Param.P;
    double etaF = gISPparam->tonemap_Param.etaF;
    double etaC = gISPparam->tonemap_Param.etaC;
    double BPCmax = gISPparam->tonemap_Param.BPCmax;
    double BPCmin = gISPparam->tonemap_Param.BPCmin;
    float shadow_boost = gISPparam->tonemap_Param.shadow_boost;         // 用户参数
    float brightness_ratio = gISPparam->tonemap_Param.brightness_ratio; // 用户参数 (如 1.0)
    float contrast_k = gISPparam->tonemap_Param.contrast_k;             // 用户参数 (如 1.0)
    float light_compress = gISPparam->tonemap_Param.light_compress;
    int W = gISPparam->rawinfo.W / 2;
    int H = gISPparam->rawinfo.H / 2;
    int N = H * W;
    int maxsize = (1 << gISPparam->tonemap_Param.TMPoutbit)-1;
    unsigned short* result = (unsigned short*)malloc(sizeof(unsigned short) * N);
    double* logimage = (double*)malloc(sizeof(double) * N);
    double* DPFlog = (double*)malloc(sizeof(double) * N);
    double* DPClog = (double*)malloc(sizeof(double) * N);
    double* BPFlog = (double*)malloc(sizeof(double) * N);
    double* BPlog = (double*)malloc(sizeof(double) * N);
    double* BPC_temp = (double*)malloc(sizeof(double) * N); // 存储基础映射结果用于统计

    double maxBPlog = -1e9, minBPlog = 1e9;

    // ---------- Step 1: Log transform ----------
#pragma omp parallel for reduction(max:maxBPlog) reduction(min:minBPlog)
    for (int i = 0; i < N; i++) {
        logimage[i] = (rawdata_y[i] <= 0) ? 0.0 : fast_log10((double)rawdata_y[i]);
        if (logimage[i] > maxBPlog) maxBPlog = logimage[i];
        if (logimage[i] < minBPlog) minBPlog = logimage[i];
    }

    // ---------- Step 2: Guided filtering ----------
    GuidedFilter(logimage, BPFlog, W, H, 1, 0.2);
#pragma omp parallel for
    for (int i = 0; i < N; i++) DPFlog[i] = logimage[i] - BPFlog[i];

    GuidedFilter(BPFlog, BPlog, W, H, 2, 1);
#pragma omp parallel for
    for (int i = 0; i < N; i++) DPClog[i] = BPFlog[i] - BPlog[i];

    // ---------- Step 3: Normalize & Base Mapping Statistics ----------
    double log_range = maxBPlog - minBPlog;
    if (log_range < 1e-6) log_range = 1e-6;
    double beta = -maxBPlog;
    double alpha = tauR / log_range;

    double BPmin = fast_pow10((minBPlog + beta) * alpha);
    double BPmax = fast_pow10((maxBPlog + beta) * alpha);
    double logP = fast_log10(P);
    double log1pP = fast_log10(1.0 + P);
    double denom = log1pP - logP;
    if (fabs(denom) < 1e-8) denom = 1e-8;

    double sum_BPC = 0.0;
#pragma omp parallel for reduction(+:sum_BPC)
    for (int i = 0; i < N; i++) {
        double cur_BPlog = (BPlog[i] + beta) * alpha;
        double BP = fast_pow10(cur_BPlog);
        double diff = BPmax - BPmin;
        if (diff < 1e-8) diff = 1e-8;
        double tmp = (BP - BPmin) / diff + P;
        if (tmp < 1e-7) tmp = 1e-7;

        // 计算基础 BPC 映射
        BPC_temp[i] = (BPCmax - BPCmin) * (fast_log10(tmp) - logP) / denom + BPCmin;
        if (BPC_temp[i] < 0) BPC_temp[i] = 0;
        if (BPC_temp[i] > 1.0) BPC_temp[i] = 1.0;

        sum_BPC += BPC_temp[i];
    }

    // ---------- Step 4: 自适应埃尔米特曲线生成 ----------
 // 1. 统计 256 级直方图
    int hist[256] = { 0 };
    for (int i = 0; i < N; i++) {
        int idx = (int)(BPC_temp[i] * 255); // 假设输入已归一化
        hist[idx]++;
    }

    // 2. 计算 CDF 找到中值点 L50
    int sum = 0;
    double L50 = 0.1; // 默认值
    for (int i = 0; i < 256; i++) {
        sum += hist[i];
        if (sum >= N * 0.5) { // 找到 50% 像素所在的亮度
            L50 = i / 255.0;
            break;
        }
    }

    // 3. 动态微调 P 和 样条锚点 Pin
    double adaptive_P = L50 * 0.8; // 这是一个简单的线性映射关系
    double P_in = L50;             // 将样条锚点直接锁定在图像中点

    // P_out 为相对亮度调整。brightness_ratio = 1.0 时不改变 P
    // 使用线性增量避免像乘法那样导致高亮区过快溢出
    double P_out = P_in + (brightness_ratio - 1.0f) * 0.2f;
    P_out = (P_out < 0.05) ? 0.05 : (P_out > 0.95 ? 0.95 : P_out);

    // 斜率设置
    double m_start = 1.0 + shadow_boost; // 暗部提亮
    double m_pivot = contrast_k;         // 旋转中心对比度
    double m_end = light_compress;                  // 高光收敛

    HermiteCoeffs segA = getHermiteCoeffs(0.0, P_in, 0.0, P_out, m_start, m_pivot);
    HermiteCoeffs segB = getHermiteCoeffs(P_in, 1.0, P_out, 1.0, m_pivot, m_end);

    double splineLUT[1024];
    for (int i = 0; i < 1024; i++) {
        double x = i / 1023.0;
        if (x < P_in) {
            double t = x;
            splineLUT[i] = segA.a * t * t * t + segA.b * t * t + segA.c * t + segA.d;
        }
        else {
            double t = x - P_in;
            splineLUT[i] = segB.a * t * t * t + segB.b * t * t + segB.c * t + segB.d;
        }
        if (splineLUT[i] < 0) splineLUT[i] = 0;
        if (splineLUT[i] > 1.0) splineLUT[i] = 1.0;
    }

    // ---------- Step 5: Final Loop with Fine-tuning ----------
#pragma omp parallel for
    for (int i = 0; i < N; i++) {
        // 1. 增益控制 SG
        double cur_BPlog = (BPlog[i] + beta) * alpha;
        double SG = (-0.4 * cur_BPlog > 1.0) ? (-0.4 * cur_BPlog) : 1.0;
        if (SG > 4.0) SG = 4.0;

        // 2. 细节层合成 (含原有的黑点保护)
        double detail_log = etaF * SG * DPFlog[i] + etaC * SG * DPClog[i];


        // 3. 基础映射 + 埃尔米特微调
        // BPC_temp[i] 是已经映射到 0~1 的基础层
        double BPC_finetuned = splineLUT[(int)(BPC_temp[i] * 1023.0)];
        double bright_th = 0.7;
        if (BPC_finetuned > bright_th) {
            double weight = (BPC_finetuned - bright_th) / (1.0 - bright_th);
            // 如果 detail 是负的（产生黑边的元凶），则根据亮度强行拉回 0
            if (detail_log < 0) {
                detail_log = detail_log * (1.0 - weight * 0.9); // 压制 90% 的负细节
            }
        }

        // --- 【新增：伪色压制逻辑（高光去饱和预处理）】 ---
        // 如果该点非常接近饱和，我们可以强制让 DP 趋向于 1.0（即对数域趋向 0）
        // 这样这一点的颜色将完全由平滑的 Base 层决定，避免 Demosaic 插值出怪色
        if (BPC_finetuned > 0.9) {
            detail_log *= 0.1;
        }
        double DP = fast_pow10(detail_log);
        // 4. 最终合成
        double YC = BPC_finetuned * DP;
        double val = YC * maxsize;

        if (val > maxsize) val = maxsize;
        else if (val < 0) val = 0;

        result[i] = (unsigned short)val;
    }

    std::free(logimage);
    std::free(DPFlog);
    std::free(DPClog);
    std::free(BPFlog);
    std::free(BPlog);
    std::free(BPC_temp);
    return result;
}

// ---------- 1. 工业级纯低分辨率空间无缝算子 ----------
static void FG_Downsample2x_Generic(const double* src, double* dst, int W, int H) {
    int dstW = (W + 1) / 2;
    int dstH = (H + 1) / 2;

#pragma omp parallel for
    for (int y = 0; y < dstH; ++y) {
        int srcY0 = y * 2;
        int srcY1 = min(srcY0 + 1, H - 1); // 奇数边界自动边界镜像

        for (int x = 0; x < dstW; ++x) {
            int srcX0 = x * 2;
            int srcX1 = min(srcX0 + 1, W - 1);

            dst[y * dstW + x] = (src[srcY0 * W + srcX0] +
                src[srcY0 * W + srcX1] +
                src[srcY1 * W + srcX0] +
                src[srcY1 * W + srcX1]) * 0.25;
        }
    }
}

// 【绝对几何中心对齐】双线性上采样 (不再受制于 canvas 比例，无相位漂移)
// 【通用中心对齐版】双线性上采样 —— 动态自动计算任意缩放比
static void FG_UpsampleBilinear_Generic(const double* src, double* dst, int srcW, int srcH, int dstW, int dstH) {
    // 动态计算水平和垂直方向的真实缩放步长
    double scaleX = (double)srcW / dstW;
    double scaleY = (double)srcH / dstH;

#pragma omp parallel for
    for (int y = 0; y < dstH; ++y) {
        // 标准中心对齐映射：将大图的像素中心 (y + 0.5) 反映射回小图空间，再减去 0.5 得到左上角索引
        double srcY = (y + 0.5) * scaleY - 0.5;
        int y0 = max(0, min((int)floor(srcY), srcH - 1));
        int y1 = min(y0 + 1, srcH - 1);
        double v = max(0.0, min(1.0, srcY - y0)); // 保证权重在 [0, 1]

        for (int x = 0; x < dstW; ++x) {
            double srcX = (x + 0.5) * scaleX - 0.5;
            int x0 = max(0, min((int)floor(srcX), srcW - 1));
            int x1 = min(x0 + 1, srcW - 1);
            double u = max(0.0, min(1.0, srcX - x0));

            // 四点插值
            dst[y * dstW + x] = (1.0 - u) * (1.0 - v) * src[y0 * srcW + x0] +
                u * (1.0 - v) * src[y0 * srcW + x1] +
                (1.0 - u) * v * src[y1 * srcW + x0] +
                u * v * src[y1 * srcW + x1];
        }
    }
}

// 空间解耦的一维分离式 BoxBlur (同样做边界守护)
static void FG_BoxBlur_Space(const double* src, double* dst, int W, int H, int r) {
    std::vector<double> tmp(W * H);
#pragma omp parallel for
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            double sum = 0.0;
            for (int k = -r; k <= r; ++k) {
                sum += src[y * W + max(0, min(W - 1, x + k))];
            }
            tmp[y * W + x] = sum / (2 * r + 1);
        }
    }
#pragma omp parallel for
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            double sum = 0.0;
            for (int k = -r; k <= r; ++k) {
                sum += tmp[max(0, min(H - 1, y + k)) * W + x];
            }
            dst[y * W + x] = sum / (2 * r + 1);
        }
    }
}

static void FG_ComputeCoeffs_Space(const double* I, double* out_a, double* out_b, int W, int H, int r, double eps) {
    int N = W * H;
    std::vector<double> II(N);
#pragma omp parallel for
    for (int i = 0; i < N; ++i) II[i] = I[i] * I[i];

    std::vector<double> mean_I(N);
    std::vector<double> mean_II(N);
    FG_BoxBlur_Space(I, mean_I.data(), W, H, r);
    FG_BoxBlur_Space(II.data(), mean_II.data(), W, H, r);

#pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        double var_I = mean_II[i] - mean_I[i] * mean_I[i];
        if (var_I < 0.0) var_I = 0.0;
        out_a[i] = var_I / (var_I + eps);
        out_b[i] = mean_I[i] - out_a[i] * mean_I[i];
    }
}

static void FG_PostSmoothCoeffs(double* arr, int W, int H) {
    std::vector<double> tmp(W * H);
#pragma omp parallel for
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int xl = max(0, x - 1);
            int xr = min(W - 1, x + 1);
            tmp[y * W + x] = (arr[y * W + xl] + arr[y * W + x] + arr[y * W + xr]) / 3.0;
        }
    }
#pragma omp parallel for
    for (int y = 0; y < H; ++y) {
        int yt = max(0, y - 1);
        int yb = min(H - 1, y + 1);
        for (int x = 0; x < W; ++x) {
            arr[y * W + x] = (tmp[yt * W + x] + tmp[y * W + x] + tmp[yb * W + x]) / 3.0;
        }
    }
}

// ---------- 2. 基于“🚀 完美重构版”精修后的 ELTM 函数 ----------
unsigned short* ELTM_pyr(int* rawdata_y, stISPParams* gISPparam)
{
    double tauR = gISPparam->tonemap_Param.tauR;
    double P = gISPparam->tonemap_Param.P;
    float shadow_boost = gISPparam->tonemap_Param.shadow_boost;
    float brightness_ratio = gISPparam->tonemap_Param.brightness_ratio;
    float contrast_k = gISPparam->tonemap_Param.contrast_k;
    float light_compress = gISPparam->tonemap_Param.light_compress;

    double eta[2] = { gISPparam->tonemap_Param.etaF, gISPparam->tonemap_Param.etaC };
    double edge_sigma_r[2] = { 0.04, 0.18 };
    double eps[2] = { edge_sigma_r[0] * edge_sigma_r[0], edge_sigma_r[1] * edge_sigma_r[1] };

    // 原始输入的真实满分辨率宽高
    int W = gISPparam->rawinfo.W / 2;
    int H = gISPparam->rawinfo.H / 2;
    int N = H * W;
    int maxsize = (1 << gISPparam->tonemap_Param.TMPoutbit) - 1;

    unsigned short* result = (unsigned short*)malloc(sizeof(unsigned short) * N);
    double* logimage = (double*)malloc(sizeof(double) * N);

    double* pyr_Detail0 = (double*)malloc(sizeof(double) * N);
    double* pyr_Detail1 = (double*)malloc(sizeof(double) * N);
    double* pyr_Base0 = (double*)malloc(sizeof(double) * N);
    double* pyr_Base1 = (double*)malloc(sizeof(double) * N);
    double* BPC_temp = (double*)malloc(sizeof(double) * N);

    double maxBPlog = -1e9, minBPlog = 1e9;

#pragma omp parallel for reduction(max:maxBPlog) reduction(min:minBPlog)
    for (int i = 0; i < N; i++) {
        logimage[i] = (rawdata_y[i] <= 0) ? 0.0 : fast_log10((double)rawdata_y[i]);
        if (logimage[i] > maxBPlog) maxBPlog = logimage[i];
        if (logimage[i] < minBPlog) minBPlog = logimage[i];
    }

    // ---------- Step 2: 金字塔动态分配（全面引入 Ceil 机制） ----------

    // 【1/2 空间分配】
    int W_sub1 = (W + 1) / 2;
    int H_sub1 = (H + 1) / 2;
    std::vector<double> img_sub1(W_sub1 * H_sub1);
    std::vector<double> a_sub1(W_sub1 * H_sub1), b_sub1(W_sub1 * H_sub1);
    std::vector<double> base_sub1(W_sub1 * H_sub1);

    FG_Downsample2x_Generic(logimage, img_sub1.data(), W, H);
    FG_ComputeCoeffs_Space(img_sub1.data(), a_sub1.data(), b_sub1.data(), W_sub1, H_sub1, 2, eps[0]);
#pragma omp parallel for
    for (int i = 0; i < W_sub1 * H_sub1; ++i) {
        base_sub1[i] = a_sub1[i] * img_sub1[i] + b_sub1[i];
    }

    // 【1/4 空间分配】
    int W_sub2 = (W_sub1 + 1) / 2;
    int H_sub2 = (H_sub1 + 1) / 2;
    std::vector<double> img_sub2(W_sub2 * H_sub2);
    std::vector<double> a_sub2(W_sub2 * H_sub2), b_sub2(W_sub2 * H_sub2);

    FG_Downsample2x_Generic(base_sub1.data(), img_sub2.data(), W_sub1, H_sub1);
    FG_ComputeCoeffs_Space(img_sub2.data(), a_sub2.data(), b_sub2.data(), W_sub2, H_sub2, 3, eps[1]);

    // 【满分辨率重建流】
    std::vector<double> full_a0(N), full_b0(N);
    std::vector<double> full_a1(N), full_b1(N);

    // 调用通用对齐重构算子
    FG_UpsampleBilinear_Generic(a_sub1.data(), full_a0.data(), W_sub1, H_sub1, W, H);
    FG_UpsampleBilinear_Generic(b_sub1.data(), full_b0.data(), W_sub1, H_sub1, W, H);
    FG_UpsampleBilinear_Generic(a_sub2.data(), full_a1.data(), W_sub2, H_sub2, W, H);
    FG_UpsampleBilinear_Generic(b_sub2.data(), full_b1.data(), W_sub2, H_sub2, W, H);

    FG_PostSmoothCoeffs(full_a1.data(), W, H);
    FG_PostSmoothCoeffs(full_b1.data(), W, H);

#pragma omp parallel for
    for (int i = 0; i < N; i++) {
        pyr_Base0[i] = full_a0[i] * logimage[i] + full_b0[i];
        pyr_Base1[i] = full_a1[i] * pyr_Base0[i] + full_b1[i];

        pyr_Detail0[i] = logimage[i] - pyr_Base0[i];
        pyr_Detail1[i] = pyr_Base0[i] - pyr_Base1[i];
    }
    // ---------- Step 3: Normalize & Base Mapping Statistics ----------
    double log_range = maxBPlog - minBPlog;
    if (log_range < 1e-6) log_range = 1e-6;
    double beta = -maxBPlog;
    double alpha = tauR / log_range;

    double BPmin = fast_pow10((minBPlog + beta) * alpha);
    double BPmax = fast_pow10((maxBPlog + beta) * alpha);
    double logP = fast_log10(P);
    double log1pP = fast_log10(1.0 + P);
    double denom = log1pP - logP;
    if (fabs(denom) < 1e-8) denom = 1e-8;

#pragma omp parallel for
    for (int i = 0; i < N; i++) {
        double cur_BPlog = (pyr_Base1[i] + beta) * alpha;
        double BP = fast_pow10(cur_BPlog);
        double diff = BPmax - BPmin;
        if (diff < 1e-8) diff = 1e-8;
        double tmp = (BP - BPmin) / diff + P;
        if (tmp < 1e-7) tmp = 1e-7;

        BPC_temp[i] = (gISPparam->tonemap_Param.BPCmax - gISPparam->tonemap_Param.BPCmin) * (fast_log10(tmp) - logP) / denom + gISPparam->tonemap_Param.BPCmin;
        BPC_temp[i] = max(0.0, min(1.0, BPC_temp[i]));
    }

    // ---------- Step 4: 自适应埃尔米特曲线生成（还原第一版纯净逻辑） ----------
    int hist[256] = { 0 };
    for (int i = 0; i < N; i++) {
        int idx = (int)(BPC_temp[i] * 255);
        hist[max(0, min(255, idx))]++;
    }

    int sum = 0;
    double L50 = 0.1;
    for (int i = 0; i < 256; i++) {
        sum += hist[i];
        if (sum >= N * 0.5) {
            L50 = i / 255.0;
            break;
        }
    }

    double P_in = L50;
    double P_out = P_in + (brightness_ratio - 1.0f) * 0.2f;
    P_out = max(0.05, min(0.95, P_out));

    double m_start = 1.0 + shadow_boost;
    double m_pivot = contrast_k;
    double m_end = light_compress;

    HermiteCoeffs segA = getHermiteCoeffs(0.0, P_in, 0.0, P_out, m_start, m_pivot);
    HermiteCoeffs segB = getHermiteCoeffs(P_in, 1.0, P_out, 1.0, m_pivot, m_end);

    double splineLUT[1024];
    for (int i = 0; i < 1024; i++) {
        double x = i / 1023.0;
        if (x < P_in) {
            splineLUT[i] = segA.a * x * x * x + segA.b * x * x + segA.c * x + segA.d;
        }
        else {
            double t = x - P_in;
            splineLUT[i] = segB.a * t * t * t + segB.b * t * t + segB.c * t + segB.d;
        }
        splineLUT[i] = max(0.0, min(1.0, splineLUT[i]));
    }

    // ---------- Step 5: Final 深度重构（还原第一版逻辑） ----------
#pragma omp parallel for
    for (int i = 0; i < N; i++) {
        double cur_BPlog = (pyr_Base1[i] + beta) * alpha;
        double SG = (-0.4 * cur_BPlog > 1.0) ? (-0.4 * cur_BPlog) : 1.0;
        if (SG > 4.0) SG = 4.0;

        double detail_log = eta[0] * SG * pyr_Detail0[i] +
            eta[1] * SG * pyr_Detail1[i];

        // 抗锯齿边缘硬门限软化
        double raw_base_diff = logimage[i] - pyr_Base1[i];
        if (std::fabs(raw_base_diff) > 0.15) {
            double aa_weight = std::exp(-(std::fabs(raw_base_diff) - 0.15) * 2.0);
            detail_log *= (0.3 + 0.7 * aa_weight);
        }

        double BPC_finetuned = splineLUT[(int)(BPC_temp[i] * 1023.0)];

        if (detail_log < 0) {
            if (raw_base_diff > 0.1) {
                double halo_suppress_factor = std::exp(-raw_base_diff * 5.0);
                detail_log *= halo_suppress_factor;
            }
            double dark_th = 0.35;
            if (BPC_finetuned < dark_th) {
                double weight = (dark_th - BPC_finetuned) / dark_th;
                detail_log = detail_log * (1.0 - weight * 0.92);
            }
        }

        if (BPC_finetuned > 0.9) {
            detail_log *= 0.05;
        }

        double DP = fast_pow10(detail_log);
        double YC = BPC_finetuned * DP;

        double safety_floor = BPC_finetuned * 0.25;
        if (YC < safety_floor) YC = safety_floor;

        double val = YC * maxsize;
        result[i] = (unsigned short)max(0.0, min((double)maxsize, val));
    }

    free(logimage);
    free(pyr_Detail0);
    free(pyr_Detail1);
    free(pyr_Base0);
    free(pyr_Base1);
    free(BPC_temp);

    return result;
}
/******************************************************/
int Run_tmp(stISPParams* gISPparam, u32* decompanddata,u16 *tmpdata)
{
    int H = gISPparam->rawinfo.H;
    int W = gISPparam->rawinfo.W;
    eBayerPattern bayerpattern = gISPparam->rawinfo.BayerType;
    int imagesize = H * W;
    int* rawdata_y = (int*)malloc(imagesize * sizeof(int) / 4);
    std::memset(rawdata_y, 0, imagesize * sizeof(int) / 4);
    convertTomaxbayer(decompanddata, rawdata_y, H, W, bayerpattern);
    //GetYdata(rawdata_out24, rawdata_y, Hight, weight);
    unsigned short* result_y = (unsigned short*)malloc(imagesize * sizeof(unsigned short) / 4);
    int Hight = H / 2;
    int weight = W / 2;
    result_y = ELTM_pyr(rawdata_y, gISPparam);
    float lamda = gISPparam->tonemap_Param.lamda;
    u16* resultWhole = (u16*)malloc(sizeof(u16) * Hight * weight * 4);
    int maxsize = (1 << gISPparam->tonemap_Param.TMPoutbit)-1 ;
    for (int i = 0; i < Hight; i++)
    {
        for (int j = 0; j < weight; j++)
        {
            tmpdata[2 * i * 2 * weight + 2 * j] = CLIP(pow(0.001 + decompanddata[2 * i * 2 * weight + 2 * j] / (rawdata_y[i * weight + j] + 0.001), lamda) * result_y[i * weight + j], 0, maxsize);
            tmpdata[(2 * i + 1) * 2 * weight + 2 * j] = CLIP(pow(0.001 + decompanddata[(2 * i + 1) * 2 * weight + 2 * j] / (rawdata_y[i * weight + j] + 0.001), lamda) * result_y[i * weight + j], 0, maxsize);
            tmpdata[2 * i * 2 * weight + 2 * j + 1] = CLIP(pow(0.001 + decompanddata[2 * i * 2 * weight + 2 * j + 1] / (rawdata_y[i * weight + j] + 0.001), lamda) * result_y[i * weight + j], 0, maxsize);
            tmpdata[(2 * i + 1) * 2 * weight + 2 * j + 1] = CLIP(pow(0.001 + decompanddata[(2 * i + 1) * 2 * weight + 2 * j + 1] / (rawdata_y[i * weight + j] + 0.001), lamda) * result_y[i * weight + j], 0, maxsize);
        }
    }


    /*test*/
    char path_out[] = "TMPout_put.raw";
    FILE* p_out = NULL;
    int ret = fopen_s(&p_out, path_out, "wb+");
    fwrite(tmpdata, sizeof(u16), imagesize, p_out);
   fclose(p_out);
    return 0;
}
