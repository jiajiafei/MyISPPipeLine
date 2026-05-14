#include<iostream>
#include"DPC.h"
void manual_sort8(u32* p) {
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7 - i; j++) {
            if (p[j] > p[j + 1]) {
                u32 temp = p[j];
                p[j] = p[j + 1];
                p[j + 1] = temp;
            }
        }
    }
}



int DPC_run(stISPParams* ispParam, u32* data)
{
    int W = ispParam->rawinfo.W;
    int H = ispParam->rawinfo.H;
    int base_threshold = ispParam->dpc_Param.threshold;
    int N = W * H;

    u32* temp_data = (u32*)malloc(sizeof(u32) * N);
    if (!temp_data) return -1;

    memcpy(temp_data, data, sizeof(u32) * N);

#pragma omp parallel for collapse(2)
    for (int y = 2; y < H - 2; y++) {
        for (int x = 2; x < W - 2; x++) {
            int idx = y * W + x;
            u32 center = data[idx];

            // 获取同色邻域 (Bayer 跨度为 2)
            u32 p[8];
            p[0] = data[(y - 2) * W + (x - 2)]; p[1] = data[(y - 2) * W + x]; p[2] = data[(y - 2) * W + (x + 2)];
            p[3] = data[y * W + (x - 2)];                                   p[4] = data[y * W + (x + 2)];
            p[5] = data[(y + 2) * W + (x - 2)]; p[6] = data[(y + 2) * W + x]; p[7] = data[(y + 2) * W + (x + 2)];

            u32 max_v = p[0], min_v = p[0];
            unsigned long long sum_v = 0;
            for (int k = 0; k < 8; k++) {
                if (p[k] > max_v) max_v = p[k];
                if (p[k] < min_v) min_v = p[k];
                sum_v += p[k];
            }

            // --- 核心逻辑改进：边缘保护 ---
            // 计算邻域本身的波动（梯度）
            u32 diff_neighbor = max_v - min_v;

            // 动态调整阈值：如果邻域本身起伏很大，说明是边缘纹理，大幅提高判定门限
            // 这样可以防止在路灯边缘、建筑轮廓处误触发 DPC
            u32 dynamic_th = base_threshold;
            if (diff_neighbor > base_threshold) {
                dynamic_th = base_threshold + (diff_neighbor >> 1);
            }

            // 只有显著超出动态阈值才判定为坏点
            if (center > max_v + dynamic_th || (min_v > dynamic_th && center < min_v - dynamic_th)) {
                // 坏点校正：使用均值
                temp_data[idx] = (u32)(sum_v >> 3);
            }
            else {
                // 非坏点：严格保持原始数值，不做任何平滑
                temp_data[idx] = center;
            }
        }
    }

    memcpy(data, temp_data, sizeof(u32) * N);
    free(temp_data);
    return 0;
}