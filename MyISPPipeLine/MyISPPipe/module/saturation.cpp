#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include"saturation.h"

typedef unsigned char u8;

int Run_saturation(stISPParams* gISPparam, u8* Rgbdata, u8* rgb8bit)
{
    int total_pixels = gISPparam->rawinfo.W * gISPparam->rawinfo.H;
    
    // 1. 将浮点系数定点化（放大 2^14 = 16384 倍）
    // 常规 RGB 转 Y 的权重 (BT.601): Y = 0.299R + 0.587G + 0.114B
    int64_t w_r = 4900;   // 0.299 * 16384
    int64_t w_g = 9617;   // 0.587 * 16384
    int64_t w_b = 1867;   // 0.114 * 16384

    // 2. 结合 strength 计算变换系数
    // 算法逻辑：默认以 256 为 base，所以实际增益（浮点）为 strength / 256.0
    // 我们在定点化中要综合处理： Y + strength * (Color - Y)
    int64_t s = gISPparam->saturation_Param.strength; // 基础增益（放大256倍）

    // 循环遍历所有像素
    for (int i = 0; i < total_pixels; i++) 
    {
        int r = Rgbdata[i + 2* total_pixels];
        int g = Rgbdata[i];
        int b = Rgbdata[i + total_pixels];

        // 计算当前像素的亮度 Y（放大 16384 倍）
        int64_t y_16384 = w_r * r + w_g * g + w_b * b;

        // 根据 YUV 空间的饱和度公式：
        // Out = Y + S * (In - Y) => (Y * 256 + s * (In * 16384 - Y)) / (16384 * 256)
        // 展开变形：Out_scaled = (Y_16384 * (256 - s) + s * In * 16384)
        // 最终右移 22 位 (14位定点 + 8位Base)
        int64_t common_part = y_16384 * (256 - s);

        int64_t r_out = (common_part + s * r * 16384) >> 22;
        int64_t g_out = (common_part + s * g * 16384) >> 22;
        int64_t b_out = (common_part + s * b * 16384) >> 22;

        // 截断到 8bit 范围
        rgb8bit[i + +2 * total_pixels] = (uint8_t)CLIP(r_out, 0, 255);
        rgb8bit[i ] = (uint8_t)CLIP(g_out, 0, 255);
        rgb8bit[i + total_pixels] = (uint8_t)CLIP(b_out, 0, 255);



    }
    if (gISPparam->rawinfo.ispmode == ISPCombined)
    {
        debug_write8bit_image(gISPparam->rawinfo.H, gISPparam->rawinfo.W, rgb8bit, "HDR-11-saturation-8bit.jpg");
    }
    else
    {
        debug_write8bit_image(gISPparam->rawinfo.H, gISPparam->rawinfo.W, rgb8bit, "Linear-11-saturation-8bit.jpg");
    }
    return 0;
}
