#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include"sharpen.h"

typedef unsigned char u8;
float* make_pad_float(
    const float* image, int H, int W,
    int pad_H, int pad_W,
    int* out_H, int* out_W)
{
    *out_H = H + 2 * pad_H;
    *out_W = W + 2 * pad_W;
    int newH = *out_H;
    int newW = *out_W;
    float* padded = (float*)malloc(newH * newW * sizeof(float));

    for (int y = 0; y < newH; ++y) {
        int srcY = y - pad_H;
        if (srcY < 0) srcY = 0;
        if (srcY >= H) srcY = H - 1;

        for (int x = 0; x < newW; ++x) {
            int srcX = x - pad_W;
            if (srcX < 0) srcX = 0;
            if (srcX >= W) srcX = W - 1;

            padded[y * newW + x] = image[srcY * W + srcX];
        }
    }
    return padded;
}

float* fast_gaussi_blur_float(
    const float* src,
    int H, int W,
    int radius,
    float sigma,
    float ratio)
{
    // 生成高斯滤波核
    int filter_size = 2 * radius + 1;
    float* filter = (float*)malloc(filter_size * sizeof(float));
    float sum = 0.0f;
    for (int i = -radius; i <= radius; ++i) {
        filter[i + radius] = (1.0f / sigma) * expf(-(i * i) / (2.0f * sigma * sigma));
        sum += filter[i + radius];
    }
    for (int i = 0; i < filter_size; ++i)
        filter[i] = filter[i] * ratio / sum;

    // pad 图像
    int padH, padW;
    float* src_pad = make_pad_float(src, H, W, radius, radius, &padH, &padW);

    // 中间缓冲
    float* temp = (float*)malloc(padH * padW * sizeof(float));
    memcpy(temp, src_pad, padH * padW * sizeof(float));

    // --- 水平卷积 ---
    for (int y = 0; y < padH; ++y) {
        for (int x = radius; x < padW - radius; ++x) {
            float sumv = 0.0f;
            for (int k = -radius; k <= radius; ++k)
                sumv += filter[k + radius] * src_pad[y * padW + (x + k)];
            temp[y * padW + x] = sumv;
        }
    }

    // --- 垂直卷积 ---
    float* result = (float*)malloc(H * W * sizeof(float));
    for (int y = radius; y < padH - radius; ++y) {
        for (int x = radius; x < padW - radius; ++x) {
            float sumv = 0.0f;
            for (int k = -radius; k <= radius; ++k)
                sumv += filter[k + radius] * temp[(y + k) * padW + x];
            result[(y - radius) * W + (x - radius)] = sumv;
        }
    }

    free(filter);
    free(src_pad);
    free(temp);
    return result;
}
float* pyramid_downsample_float(const float* src, int H, int W) {

    int newH = H / 2;
    int newW = W / 2;
    float* out = (float*)malloc(sizeof(float) * newH * newW);
    for (int i = 0; i < newH; i++)
    {
        for (int j = 0; j < newW; j++)
        {
            out[i * newW + j] = src[i * 2 * W + 2 * j];
        }
    }
    return out;
}
typedef struct {

    float** layers;
    int* H;
    int* W;
    int num_layers;


}GaussPyramid;

GaussPyramid* build_gaussi_pyramid_float(const float* source, int H, int W, int layers_num)
{
    int factor = 1 << layers_num;

    // 向上补齐，而不是向下截断
    int new_H = ((H + factor - 1) / factor) * factor;
    int new_W = ((W + factor - 1) / factor) * factor;

    // 创建补齐后的 base 层
    float* base = (float*)calloc(new_H * new_W, sizeof(float)); // 自动置0
    for (int y = 0; y < H; ++y)
        memcpy(base + y * new_W, source + y * W, W * sizeof(float));

    GaussPyramid* pyramid = (GaussPyramid*)malloc(sizeof(GaussPyramid));
    pyramid->num_layers = layers_num;
    pyramid->layers = (float**)malloc(layers_num * sizeof(float*));
    pyramid->H = (int*)malloc(layers_num * sizeof(int));
    pyramid->W = (int*)malloc(layers_num * sizeof(int));

    pyramid->layers[0] = base;
    pyramid->H[0] = new_H;
    pyramid->W[0] = new_W;

    float* prev = base;
    int curH = new_H;
    int curW = new_W;

    // 构造后续层
    for (int i = 1; i < layers_num; ++i) {
        // 高斯模糊
        float* blurred = fast_gaussi_blur_float(prev, curH, curW, 2, 1.0f, 1.0f);

        // 下采样
        float* down = pyramid_downsample_float(blurred, curH, curW);
        int downH = curH / 2;
        int downW = curW / 2;

        free(blurred);

        pyramid->layers[i] = down;
        pyramid->H[i] = downH;
        pyramid->W[i] = downW;

        prev = down;
        curH = downH;
        curW = downW;
    }

    return pyramid;

}
float* pyramid_upsample_float(const float* src, int H, int W) {

    int newH = H * 2;
    int newW = W * 2;
    float* out = (float*)malloc(sizeof(float) * newH * newW);
    for (int i = 0; i < newH; i++)
    {
        for (int j = 0; j < newW; j++)
        {
            if ((i % 2 == 0) && (j % 2 == 0))
            {
                out[i * newW + j] = src[W * (i / 2) + j / 2];
            }
            else
            {
                out[i * newW + j] = 0;
            }

        }
    }
    return out;
}

// source: 上采样后的图像数据指针
// H: 高度, W: 宽度
void pyramid_upsample_interpolate_float(float* source, int H, int W) {
    // --- 处理第一行 ---
    for (int j = 1; j < W - 1; j += 2) {
        source[j] = (source[j - 1] + source[j + 1]) / 2.0f;
    }
    source[W - 1] = source[W - 2];

    // --- 处理最后一行 ---
    float* last_row = source + (H - 1) * W;
    for (int j = 1; j < W - 1; j += 2) {
        last_row[j] = (last_row[j - 1] + last_row[j + 1]) / 2.0f;
    }
    last_row[W - 1] = last_row[W - 2];

    // --- 第一列 ---
    for (int i = 1; i < H - 1; i += 2) {
        source[i * W] = (source[(i - 1) * W] + source[(i + 1) * W]) / 2.0f;
    }

    // --- 中间行列插值 ---
    for (int i = 1; i < H - 1; ++i) {
        for (int j = 1; j < W - 1; ++j) {
            if ((i & 1) && (j & 1)) { // 奇数行奇数列
                source[i * W + j] = (source[(i - 1) * W + (j - 1)] + source[(i + 1) * W + (j - 1)] +
                    source[(i - 1) * W + (j + 1)] + source[(i + 1) * W + (j + 1)]) / 4.0f;
            }
            else if (i & 1) { // 奇数行偶数列
                source[i * W + j] = (source[(i - 1) * W + j] + source[(i + 1) * W + j]) / 2.0f;
            }
            else if (j & 1) { // 偶数行奇数列
                source[i * W + j] = (source[i * W + j - 1] + source[i * W + j + 1]) / 2.0f;
            }
        }
    }

    // --- 最后一列 ---
    for (int i = 1; i < H - 1; ++i) {
        source[i * W + (W - 1)] = source[i * W + (W - 2)];
    }
}


// 构建拉普拉斯金字塔（纯 float 实现）
float** build_laplace_pyramid_float(const GaussPyramid* gaussi_pyramid)
{
    int layers_num = gaussi_pyramid->num_layers;

    // 为每层分配 residual 存储指针
    float** laplace_pyramid = (float**)malloc(layers_num * sizeof(float*));

    // 从低分辨率往上构建
    for (int i = 0; i < layers_num - 1; ++i)
    {
        int H = gaussi_pyramid->H[i];
        int W = gaussi_pyramid->W[i];
        int nextH = gaussi_pyramid->H[i + 1];
        int nextW = gaussi_pyramid->W[i + 1];

        const float* curr = gaussi_pyramid->layers[i];
        const float* next = gaussi_pyramid->layers[i + 1];

        // 1️⃣ 上采样下一层
        float* upsampled = pyramid_upsample_float(next, nextH, nextW);

        // 2️⃣ 插值补齐
        pyramid_upsample_interpolate_float(upsampled, H, W);

        // 3️⃣ 分配 residual 层
        float* residual = (float*)malloc(H * W * sizeof(float));

        // 4️⃣ 残差计算
        for (int idx = 0; idx < H * W; ++idx)
        {
            residual[idx] = curr[idx] - upsampled[idx];
        }

        laplace_pyramid[i] = residual;

        // 释放上采样内存
        free(upsampled);
    }

    // 5️⃣ 最后一层（最高层直接复制）
    int topH = gaussi_pyramid->H[layers_num - 1];
    int topW = gaussi_pyramid->W[layers_num - 1];
    const float* top = gaussi_pyramid->layers[layers_num - 1];

    float* top_residual = (float*)malloc(topH * topW * sizeof(float));
    memcpy(top_residual, top, sizeof(float) * topH * topW);

    laplace_pyramid[layers_num - 1] = top_residual;

    return laplace_pyramid;
}

float* rebuild_image_from_laplace_pyramid_float(
    float* low_res,
    float** laplace_pyramid,
    int* layer_H,
    int* layer_W,
    int num_layers
)
{
    float* reconstructed = low_res;

    for (int i = num_layers - 2; i >= 0; --i)
    {
        int H = layer_H[i];
        int W = layer_W[i];
        int length = H * W;

        // 上采样上一层
        float* upsampled = pyramid_upsample_float(reconstructed, layer_H[i + 1], layer_W[i + 1]);


        // 插值补齐
        pyramid_upsample_interpolate_float(upsampled, H, W);

        // 分配新 reconstructed
        float* current = (float*)malloc(sizeof(float) * length);

        for (int idx = 0; idx < length; ++idx)
            current[idx] = upsampled[idx] + laplace_pyramid[i][idx];

        // 释放上一层内存
        if (i != num_layers - 2) free(reconstructed);
        free(upsampled);

        reconstructed = current;
    }

    return reconstructed; // 注意：外部需要 free() 最终图像
}
void rgb_to_yuv(const u8* R, const u8* G, const u8* B,
    u8* Y, u8* U, u8* V, int size, int maxvalue)
{
    for (int i = 0; i < size; ++i) {
        float r = R[i], g = G[i], b = B[i];
        float y = 0.299f * r + 0.587f * g + 0.114f * b;
        float u = -0.14713f * r - 0.28886f * g + 0.436f * b + 128.0f;
        float v = 0.615f * r - 0.51499f * g - 0.10001f * b + 128.0f;
        Y[i] = CLIP(y, 0, maxvalue);
        U[i] = CLIP(u, 0, maxvalue);
        V[i] = CLIP(v, 0, maxvalue);
    }
}

void yuv_to_rgb(const u8* Y, const u8* U, const u8* V,
    u8* R, u8* G, u8* B, int size, int maxvalue)
{
    for (int i = 0; i < size; ++i) {
        float y = Y[i];
        float u = U[i] - 128.0f;
        float v = V[i] - 128.0f;
        int r = (int)(y + 1.13983f * v);
        int g = (int)(y - 0.39465f * u - 0.58060f * v);
        int b = (int)(y + 2.03211f * u);
        R[i] = CLIP(r, 0, maxvalue);
        G[i] = CLIP(g, 0, maxvalue);
        B[i] = CLIP(b, 0, maxvalue);
    }
}

#if 0
//拉普拉斯滤波器，增强后整幅图像不太自然
int Run_sharpen(stISPParams* gISPparam, u8* Rgbdata, u8* yuv)
{
    int W = gISPparam->rawinfo.W;
    int H = gISPparam->rawinfo.H;
    int size = W * H;
    int maxvalue = (1 << gISPparam->sharpen_Param.sharpenbit) - 1;
    const u8* G = Rgbdata;
    const u8* B = Rgbdata + size;
    const u8* R = Rgbdata + size * 2;

    u8* Y = (u8*)malloc(size);
    u8* U = (u8*)malloc(size);
    u8* V = (u8*)malloc(size);

    // 1. RGB → YUV
    rgb_to_yuv(R, G, B, Y, U, V, size, maxvalue);
    float* input = (float*)malloc(sizeof(float) * H * W);

    for (int i = 0; i < H; i++)
    {
        for (int j=0;j<W;j++)
        {
            input[i * W + j] = (float)Y[i*W+j]/ maxvalue;
        }
    }
    int layers_num = gISPparam->sharpen_Param.layers_num;
    int N = gISPparam->sharpen_Param.N;
    float fact = gISPparam->sharpen_Param.fact;
    float sigma = gISPparam->sharpen_Param.sigma;
    float discretisation_step = gISPparam->sharpen_Param.discretisation_step;
    // 构建高斯金字塔
    GaussPyramid* gaussi_pyramid = build_gaussi_pyramid_float(input, H, W, layers_num);

    // 构建拉普拉斯金字塔
    float** laplace_pyramid = build_laplace_pyramid_float(gaussi_pyramid);

    for (int i = 0; i < N; i++)
    {
        float ref = 0.1f * i;

        // 生成 I_remap
        float* I_remap = (float*)malloc(sizeof(float) * H * W);
        for (int idx = 0; idx < H * W; ++idx)
        {
            float diff = input[idx] - ref;
            I_remap[idx] = fact * diff * expf(-diff * diff / (2.0f * sigma * sigma));
        }

        // 构建 I_remap 的高斯金字塔
        GaussPyramid* gaussi_pyramid_temp = build_gaussi_pyramid_float(I_remap, H, W, layers_num);
        float** laplace_pyramid_temp = build_laplace_pyramid_float(gaussi_pyramid_temp);

        // 融合 laplace 金字塔
        for (int L = 0; L < layers_num - 1; ++L)
        {
            int curH = gaussi_pyramid->H[L];
            int curW = gaussi_pyramid->W[L];
            int length = curH * curW;

            for (int idx = 0; idx < length; ++idx)
            {
                float gauss_val = gaussi_pyramid->layers[L][idx];
                float flag = fabsf(gauss_val - ref) < discretisation_step ? 1.0f : 0.0f;
                laplace_pyramid[L][idx] += flag * laplace_pyramid_temp[L][idx] * (1.0f - fabsf(gauss_val - ref) / discretisation_step);
            }
        }

        // 释放临时内存
        for (int L = 0; L < layers_num; ++L) free(gaussi_pyramid_temp->layers[L]);
        free(gaussi_pyramid_temp->layers);
        free(gaussi_pyramid_temp->H);
        free(gaussi_pyramid_temp->W);
        free(gaussi_pyramid_temp);

        for (int L = 0; L < layers_num - 1; ++L) free(laplace_pyramid_temp[L]);
        free(laplace_pyramid_temp);

        free(I_remap);
    }

    // 重建图像
    float* reconstructed = rebuild_image_from_laplace_pyramid_float(
        gaussi_pyramid->layers[layers_num - 1],
        laplace_pyramid,
        gaussi_pyramid->H,
        gaussi_pyramid->W,
        layers_num
    );
    for (int i=0;i<H*W;i++)
    {
        Y[i] = (u8)CLIP(reconstructed[i] * maxvalue,0,maxvalue);
    }

    u8* outG = yuv;
    u8* outB = yuv + size;
    u8* outR = yuv + 2 * size;
    yuv_to_rgb(Y, U, V, outR, outG, outB, size, maxvalue);
    debug_write8bit_image(H, W, yuv, "sharpen.jpg");
    return 0;
}
#endif

void Separable_Blur_Y(const u8* src, u8* dst, int W, int H, int K) {
    float *temp=(float *)malloc(W * H*sizeof(float));
    int radius = K / 2;
    float scale = 1.0f / K;

    // 横向滤波 (Horizontal Pass)
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float sum = 0;
            for (int k = -radius; k <= radius; k++) {
                int ix =min(max(x + k, 0), W - 1);
                sum += src[y * W + ix];
            }
            temp[y * W + x] = sum * scale;
        }
    }

    // 纵向滤波 (Vertical Pass)
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float sum = 0;
            for (int k = -radius; k <= radius; k++) {
                int iy = min(max(y + k, 0), H - 1);
                sum += temp[iy * W + x];
            }
            dst[y * W + x] = (u8)(sum * scale + 0.5f);
        }
    }
}

// 17点亮度增益查找表（对应亮度0, 16, 32...255）
static const float s_luma_gain_lut[17] = {
    0.1f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f
};

inline float get_lut_val(u8 luma, const float* lut) {
    float pos = luma / 16.0f;
    int idx = (int)pos;
    if (idx >= 16) return lut[16];
    float fract = pos - idx;
    return lut[idx] * (1.0f - fract) + lut[idx + 1] * fract;
}
static inline float soft_coring(int d, int th) {
    int ad = (d >= 0) ? d : -d; // 或者使用 abs(d)

    if (ad < th) {
        return 0.0f;
    }
    else {
        // 计算公式: sign(d) * (|d| - th)
        // 这样可以保证细节从阈值点开始是从 0 平滑增加的，而不是突然跳变
        float res = (float)(ad - th);
        return (d > 0) ? res : -res;
    }
}
int Run_sharpen(stISPParams* gISPparam, u8* Rgbdata, u8* yuv_out)
{
    int W = gISPparam->rawinfo.W;
    int H = gISPparam->rawinfo.H;
    int size = W * H;

    u8* Y = (u8*)malloc(size);
    u8* U = (u8*)malloc(size);
    u8* V = (u8*)malloc(size);
    u8* Y_org_bak = (u8*)malloc(size); // 备份原图Y用于计算增益比

    // 1. RGB -> YUV (假设由您提供的函数完成)
    rgb_to_yuv(Rgbdata + size * 2, Rgbdata, Rgbdata + size, Y, U, V, size, 255);
    memcpy(Y_org_bak, Y, size);

    // 2. 分离滤波分频
    u8* Y_blur_small = (u8*)malloc(size); // 5x5 等效
    u8* Y_blur_large = (u8*)malloc(size); // 11x11 等效
    Separable_Blur_Y(Y, Y_blur_small, W, H, 3);
    Separable_Blur_Y(Y, Y_blur_large, W, H, 7);

    // 参数从配置结构体读取
    float gain_f = gISPparam->sharpen_Param.gain_fine;
    float gain_m = gISPparam->sharpen_Param.gain_mid;
    int coring_base = gISPparam->sharpen_Param.coring_high;
    float chroma_alpha = 0.5f; // UV补偿强度，通常0.3-0.7
    int under_shot_limit = gISPparam->sharpen_Param.under_shot_limit;
    int over_shot_limit = gISPparam->sharpen_Param.over_shot_limit;
    // 3. 核心像素循环
    for (int y = 5; y < H - 5; y++) {
        for (int x = 5; x < W - 5; x++) {
            int idx = y * W + x;

            // --- A. 细节提取与软死区 ---
            float luma_w = get_lut_val(Y_blur_large[idx], s_luma_gain_lut);
            int d_f = (int)Y[idx] - (int)Y_blur_small[idx];
            int d_m = (int)Y_blur_small[idx] - (int)Y_blur_large[idx];

            int dyn_coring = (int)(coring_base * (2.0f - luma_w));


            float detail = (soft_coring(d_f, dyn_coring) * gain_f + soft_coring(d_m, dyn_coring / 2) * gain_m) * luma_w;

            // --- B. Shoot Control (极值限幅) ---
            u8 l_max = 0, l_min = 255;
            for (int fy = -2; fy <= 2; fy++) {
                for (int fx = -2; fx <= 2; fx++) {
                    u8 v = Y_org_bak[(y + fy) * W + (x + fx)];
                    if (v > l_max) l_max = v;
                    if (v < l_min) l_min = v;
                }
            }

            int y_new = max((int)l_min - under_shot_limit, min((int)Y[idx] + (int)detail, (int)l_max + over_shot_limit));
            Y[idx] = (u8)max(0, min(255, y_new));

            // --- C. UV 饱和度补偿 ---
            // 只有当亮度被增强时才补偿，且暗部不补偿以防噪点
            float y_org = (float)Y_org_bak[idx];
            if (y_org > 10.0f && y_new > y_org) {
                float gain_ratio = (float)y_new / y_org;
                // 补偿强度随亮度权重衰减，保护暗部
                float actual_alpha = chroma_alpha * luma_w;
                float uv_gain = 1.0f + actual_alpha * (gain_ratio - 1.0f);

                int u_c = (int)(((int)U[idx] - 128) * uv_gain) + 128;
                int v_c = (int)(((int)V[idx] - 128) * uv_gain) + 128;

                U[idx] = (u8)max(0, min(255, u_c));
                V[idx] = (u8)max(0, min(255, v_c));
            }
        }
    }

    // 4. 数据拷贝回输出指针
    memcpy(yuv_out, Y, size);
    memcpy(yuv_out + size, U, size);
    memcpy(yuv_out + size * 2, V, size);
    u8* outG = yuv_out;
    u8* outB = yuv_out + size;
    u8* outR = yuv_out + 2 * size;
    int maxvalue = (1 << gISPparam->sharpen_Param.sharpenbit) - 1;
    yuv_to_rgb(Y, U, V, outR, outG, outB, size, maxvalue);
    debug_write8bit_image(H, W, yuv_out, "5-sharpen.jpg");
    // 释放资源
    free(Y); free(U); free(V); free(Y_org_bak);
    free(Y_blur_small); free(Y_blur_large);
    return 0;
}