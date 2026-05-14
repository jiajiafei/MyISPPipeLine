#include<iostream>
#include"contrast.h"
int  Run_contrast(stISPParams* gISPparam, u16* Rgbdata,u8 *Rgbdata8bit)
{
	u32 m_nMax = (1 << gISPparam->contrast_Param.contrastbit) - 1;
	u32 frontblock_m_nMax= (1 << gISPparam->gamma_Param.gammabitout) - 1;
	float ContrastStrength = (float)gISPparam->contrast_Param.ContrastStrength / (float)16;
	u8 m_nMin = 0;
	int W = gISPparam->rawinfo.W;
	int H = gISPparam->rawinfo.H;
	u32 imagesize = W* H;

	float Strength = (float)gISPparam->contrast_Param.ContrastStrength / 128.0f;

	// Strength 越大，scale 越小，曲线越陡
	// 这里 0.5f 决定了最大强度，1.5f 决定了最小强度（数值可调）
	float scale = 0.5f + (1.0f / (Strength + 0.001f)) * 0.5f;
	float inner_constant = 3.141592f / (2.f * scale);
	float sin_constant = sin(inner_constant);
	float slope = (float)m_nMax / (2.f * sin_constant);
	float constant = slope * sin_constant;
	float factor = 3.141592f / (scale * (float)frontblock_m_nMax);
	float white_scale = (float)m_nMax / ((float)m_nMax );
	u8* Table = new u8[frontblock_m_nMax + 1];
	for (int k = 0; k < frontblock_m_nMax + 1; k++)
	{
		long int  tmp = ((slope * sin(factor * k - inner_constant) + constant)) * white_scale;
		Table[k] = CLIP(tmp, m_nMin, m_nMax);
	}
#pragma omp parallel for 
	for (int i = 0; i < imagesize * 3; i++)
	{
		Rgbdata8bit[i] = Table[Rgbdata[i]];
	}
	delete[]Table;

	if (gISPparam->rawinfo.ispmode == ISPCombined)
	{
		debug_write8bit_image(H, W, Rgbdata8bit, "4-contrast.jpg");
	}
	else
	{
		debug_write8bit_image(H, W, Rgbdata8bit, "4-contrast.jpg");
	}

	
	return 0;
}