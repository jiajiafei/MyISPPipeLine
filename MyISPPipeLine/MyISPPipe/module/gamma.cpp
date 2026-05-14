#include<iostream>
#include"gamma.h"
int  Run_sgamma(stISPParams* gISPparam, u16* Rgbdata)
{
    u32 m_nMaxIn = (1 << gISPparam->gamma_Param.gammabitin) - 1;   // 输入位深：通常为16bit → 65535
    u32 m_nMaxOut = (1 << gISPparam->gamma_Param.gammabitout) - 1;                                // 输出位深：12bit → 4095

    u16 cutoff = m_nMaxIn * 0.00304;
    float gamma_powe = gISPparam->gamma_Param.gamma_power;
    float gamma_toe = 12.92;
    float gamma_pow = 1 / gamma_powe;
    float gamma_fac = 1.055 * pow((float)m_nMaxIn, (1 - gamma_pow));
    float gamma_con = -0.055 * m_nMaxIn;

    u32 m_nMin = 0;
    u32 imagesize = gISPparam->rawinfo.H * gISPparam->rawinfo.W;

    unsigned short* GammaTable = new unsigned short[m_nMaxIn + 1];

    for (int k = 0; k <= (int)m_nMaxIn; k++)
    {
        double tmp;
        if (k < cutoff)
            tmp = gamma_toe * k;
        else
            tmp = gamma_fac * pow(k, gamma_pow) + gamma_con;

        // 将结果从16bit范围映射到12bit
        double out = tmp * (double)m_nMaxOut / (double)m_nMaxIn;
        GammaTable[k] = (unsigned short)CLIP(out, m_nMin, m_nMaxOut);
    }

    // 并行应用Gamma表
#pragma omp parallel for
    for (int i = 0; i < imagesize * 3; i++)
    {
        Rgbdata[i] = GammaTable[Rgbdata[i]];
    }

    delete[] GammaTable;

    if (gISPparam->rawinfo.ispmode==ISPCombined)
    {
        debug_write12bit_image(gISPparam->rawinfo.H, gISPparam->rawinfo.W, Rgbdata, "3-gamma.jpg");
    }
    else
    {
        debug_write10bit_image(gISPparam->rawinfo.H, gISPparam->rawinfo.W, Rgbdata, "3-gamma.jpg");
    }
	
	return 0;
}