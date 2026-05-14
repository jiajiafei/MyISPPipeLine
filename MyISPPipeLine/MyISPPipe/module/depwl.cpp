#include"depwl.h"
#include<iostream>
void GetFunction(int p1_x, int p2_x, int p1_y, int p2_y, int* Map)//decompanding
{

    double k = (double)(p1_y - p2_y) / (double)(p1_x - p2_x);
    double b = (double)p1_y - k * (double)p1_x;
    for (int i = p1_x; i < p2_x; i++)
    {
        *(Map + i - p1_x) = (int)(k * i + b);
    }
}
int Run_depwl(stISPParams* gISPparam, u16* Rawdata, u32* decompanddata)
{
    u8 neepoint = gISPparam->depwl_param.neepoint;
    eISPMode ispmode = gISPparam->rawinfo.ispmode;
    int maxInput = 1 << 12;

    int* X = (int*)malloc(sizeof(int) * neepoint);
    int* Y = (int*)malloc(sizeof(int) * neepoint);
    int ii = 0;
    for (size_t i = 0; i < neepoint; ++i) {
        Y[i] = gISPparam->depwl_param.y[i];
      
    }
    for (size_t i = 0; i < neepoint; ++i) {
        X[i] = gISPparam->depwl_param.x[i];
       
    }

    int* Map = (int*)malloc(maxInput * sizeof(int));
    for (int j = 0; j < neepoint - 1; j++)
    {
        int* Temp_Map = (int*)malloc((Y[j + 1] - Y[j]) * sizeof(int));
        std::memset(Temp_Map, 0, (Y[j + 1] - Y[j]) * sizeof(int));
        GetFunction(Y[j], Y[j + 1], X[j], X[j + 1], Temp_Map);
        memcpy(Map + Y[j], Temp_Map, (Y[j + 1] - Y[j]) * sizeof(int));

    }
    Map[maxInput - 1] = X[neepoint - 1];
    int bit_num = 8;
    int current_bit = gISPparam->rawinfo.rawbit;
    int DataForm = 1;//
    int minValue = 65535;
    int maxValue = 0;
    int max_indexnum = 0;
    long long countNum = 0;
    if (ispmode==ISPCombined)
    {
        for (int i = 0; i < gISPparam->rawinfo.H * gISPparam->rawinfo.W; i++)
        {
            //需要转到12bit
            if (16 == current_bit)
            {
                Rawdata[i] = (Rawdata[i]) >> 4;
            }

            decompanddata[i] = Map[Rawdata[i]];
        }
    }
    if (ispmode==ISPLinear)
    {
        for (int i = 0; i < gISPparam->rawinfo.H * gISPparam->rawinfo.W; i++)
        {
            //需要转到12bit
            if (12 == current_bit)
            {
                Rawdata[i] = (Rawdata[i]) << 4;
            }

            decompanddata[i] = Rawdata[i];
        }
    }

    free(Map);
    return 0;
}