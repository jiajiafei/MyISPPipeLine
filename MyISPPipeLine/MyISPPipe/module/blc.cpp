#include<iostream>
#include"blc.h"
int BLC_run(stISPParams *ispParam,u32 * data)
{

	
	eBayerPattern bayerPattern = ispParam->rawinfo.BayerType;
	int H = ispParam->rawinfo.H;
	int W = ispParam->rawinfo.W;

	float k_scale_1 = 0;
	float k_scale_2 = 0;
	float k_scale_3 = 0;
	float k_scale_4 = 0;
	unsigned short BlockBlc[2][2] = { 0 };
	switch (bayerPattern)
	{
	case RGGB:
		BlockBlc[0][0] = ispParam->blc_Param.blc_r;
		BlockBlc[0][1] = ispParam->blc_Param.blc_g;
		BlockBlc[1][0] = ispParam->blc_Param.blc_g;
		BlockBlc[1][1] = ispParam->blc_Param.blc_b;
		break;

	case GRBG:
		BlockBlc[0][1] = ispParam->blc_Param.blc_r;
		BlockBlc[0][0] = ispParam->blc_Param.blc_g;
		BlockBlc[1][1] = ispParam->blc_Param.blc_g;
		BlockBlc[1][0] = ispParam->blc_Param.blc_b;
		break;

	case GBRG:
		BlockBlc[1][0] = ispParam->blc_Param.blc_r;
		BlockBlc[1][1] = ispParam->blc_Param.blc_g;
		BlockBlc[0][0] = ispParam->blc_Param.blc_g;
		BlockBlc[0][1] = ispParam->blc_Param.blc_b;
		break;

	case BGGR:
		BlockBlc[1][1] = ispParam->blc_Param.blc_r;
		BlockBlc[1][0] = ispParam->blc_Param.blc_g;
		BlockBlc[0][1] = ispParam->blc_Param.blc_g;
		BlockBlc[0][0] = ispParam->blc_Param.blc_b;
		break;
	case CRGC:
		BlockBlc[0][1] = ispParam->blc_Param.blc_r;
		BlockBlc[0][0] = ispParam->blc_Param.blc_g;
		BlockBlc[1][1] = ispParam->blc_Param.blc_g;
		BlockBlc[1][0] = ispParam->blc_Param.blc_b;
		break;
	default:
		printf("This pattern is illegal!\n");
		return -1;
	}
	k_scale_1 = (float)((1 << ispParam->blc_Param.BlcBit) - 1) / (float)(((1 << ispParam->blc_Param.BlcBit) - 1) - BlockBlc[0][0]);
	k_scale_2 = (float)((1 << ispParam->blc_Param.BlcBit) - 1) / (float)(((1 << ispParam->blc_Param.BlcBit) - 1) - BlockBlc[0][1]);
	k_scale_3 = (float)((1 << ispParam->blc_Param.BlcBit) - 1) / (float)(((1 << ispParam->blc_Param.BlcBit) - 1) - BlockBlc[1][0]);
	k_scale_4 = (float)((1 << ispParam->blc_Param.BlcBit) - 1) / (float)(((1 << ispParam->blc_Param.BlcBit) - 1) - BlockBlc[1][1]);


	int h_half = H / 2;
	int w_half = W / 2;

	for (int i=0;i< h_half;i++)
	{
		for (int j=0;j< w_half;j++)
		{
			data[2 * i * w_half + 2 * j] = GetPix(data, w_half,2*i,2*j)* k_scale_1;
			data[2 * i * w_half + 2 * j+1] = GetPix(data, w_half, 2 * i, 2 * j+1) * k_scale_2;
			data[(2 * i+1) * w_half + 2 * j] = GetPix(data, w_half, 2 * i+1, 2 * j) * k_scale_3;
			data[(2 * i + 1) * w_half + 2 * j+1] = GetPix(data, w_half, 2 * i+1, 2 * j+1) * k_scale_4;
		}
	}
}