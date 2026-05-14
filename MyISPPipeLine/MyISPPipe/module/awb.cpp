#include"awb.h"

int AWB_run(stISPParams* ispParam, u32* decompanddata)
{
	eBayerPattern bayerPattern = ispParam->rawinfo.BayerType;
	int H = ispParam->rawinfo.H;
	int W = ispParam->rawinfo.W;
	int BlockBlc[2][2] = { 0 };
	float c_gain = 0;
	float R_gain = ispParam->awb_Param.r_gain;
	float B_gain = ispParam->awb_Param.b_gain;
	float w1 = ispParam->awb_Param.C_w1;
	float w2 = ispParam->awb_Param.C_w2;
	float w3 = ispParam->awb_Param.C_w3;

	int maxvalue = (1 << (ispParam->awb_Param.awbbit_af)) - 1;
	switch (bayerPattern)
	{
	case RGGB:
		BlockBlc[0][0] = ispParam->awb_Param.r_gain;
		BlockBlc[0][1] = 256;
		BlockBlc[1][0] = 256;
		BlockBlc[1][1] = ispParam->awb_Param.b_gain;
		break;

	case GRBG:
		BlockBlc[0][1] = ispParam->awb_Param.r_gain;
		BlockBlc[0][0] = 256;
		BlockBlc[1][1] = 256;
		BlockBlc[1][0] = ispParam->awb_Param.b_gain;
		break;

	case GBRG:
		BlockBlc[1][0] = ispParam->awb_Param.r_gain;
		BlockBlc[1][1] = 256;
		BlockBlc[0][0] = 256;
		BlockBlc[0][1] = ispParam->awb_Param.b_gain;
		break;

	case BGGR:
		BlockBlc[1][1] = ispParam->awb_Param.r_gain;
		BlockBlc[1][0] = 256;
		BlockBlc[0][1] = 256;
		BlockBlc[0][0] = ispParam->awb_Param.b_gain;
		break;
	case CRGC:
		
		BlockBlc[0][1] = ispParam->awb_Param.r_gain;
		BlockBlc[1][0] = 256;
		BlockBlc[1][1] = c_gain;
		BlockBlc[0][0] = c_gain;
		break;
	default:
		printf("This pattern is illegal!\n");
		return -1;
	}
	float k_scale_1 =  (float)BlockBlc[0][0]/AWB_scale;
	float k_scale_2 = (float)BlockBlc[0][1] / AWB_scale;
	float k_scale_3 = (float)BlockBlc[1][0] / AWB_scale;
	float k_scale_4 = (float)BlockBlc[1][1] / AWB_scale;
	printf("k_scale_1=%f,k_scale_1=%f,k_scale_3=%f,k_scale_4=%f\n", k_scale_1, k_scale_2, k_scale_3, k_scale_4);

	int h_half = H / 2;
	int w_half = W / 2;
	if(bayerPattern==CRGC)
	{ 
	for (int i = 0; i < h_half; i++)
	{
		for (int j = 0; j < w_half; j++)
		{


			int R = GetPix(decompanddata, W, 2 * i, 2 * j + 1);
			int c = GetPix(decompanddata, W, 2 * i, 2 * j);
			k_scale_1=B_gain+(w1*R*(R_gain-B_gain)+w2*(1-B_gain))/(c+0.001);
			k_scale_4 = k_scale_1;
			
			decompanddata[2 * i * W + 2 * j] = (u32)CLIP(GetPix(decompanddata, W, 2 * i, 2 * j) * k_scale_1,0,maxvalue);
			decompanddata[2 * i * W + 2 * j + 1] = (u32)CLIP(GetPix(decompanddata, W, 2 * i, 2 * j + 1) * k_scale_2, 0, maxvalue);
			decompanddata[(2 * i + 1) * W + 2 * j] = (u32)CLIP(GetPix(decompanddata, W, 2 * i + 1, 2 * j) * k_scale_3, 0, maxvalue);
			decompanddata[(2 * i + 1) * W + 2 * j + 1] = (u32)CLIP(GetPix(decompanddata, W, 2 * i + 1, 2 * j + 1) * k_scale_4, 0, maxvalue);
		}
	}
	}
	else
	{
		for (int i = 0; i < h_half; i++)
		{
			for (int j = 0; j < w_half; j++)
			{
		decompanddata[2 * i * W + 2 * j] = (u32)CLIP(GetPix(decompanddata, W, 2 * i, 2 * j) * k_scale_1, 0, maxvalue);
		decompanddata[2 * i * W + 2 * j + 1] = (u32)CLIP(GetPix(decompanddata, W, 2 * i, 2 * j + 1) * k_scale_2, 0, maxvalue);
		decompanddata[(2 * i + 1) * W + 2 * j] = (u32)CLIP(GetPix(decompanddata, W, 2 * i + 1, 2 * j) * k_scale_3, 0, maxvalue);
		decompanddata[(2 * i + 1) * W + 2 * j + 1] = (u32)CLIP(GetPix(decompanddata, W, 2 * i + 1, 2 * j + 1) * k_scale_4, 0, maxvalue);
			}
		}
	}
}
int AWB_run(stISPParams* ispParam, u16* Rgbdata)
{
	eBayerPattern bayerPattern = ispParam->rawinfo.BayerType;
	int H = ispParam->rawinfo.H;
	int W = ispParam->rawinfo.W;
	int imagesize = H * W;
	int maxvalue = (1 << ispParam->awb_Param.awbbit_af) - 1;
	unsigned short BlockBlc[2][2] = { 0 };
	float c_gain = 0;
	float R_gain = ispParam->awb_Param.r_gain;
	float B_gain = ispParam->awb_Param.b_gain;
	for (int i=0;i<imagesize;i++)
	{
		
		Rgbdata[i + imagesize] = CLIP(Rgbdata[i + imagesize]* B_gain,0,maxvalue);
		Rgbdata[i + 2 * imagesize] = CLIP(Rgbdata[i + 2 * imagesize]*R_gain,0,maxvalue);

	}
	return 0;
}