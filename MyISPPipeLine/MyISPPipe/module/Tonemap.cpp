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
    result_y = ELTM(rawdata_y, gISPparam);
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