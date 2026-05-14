#include"ccm.h"
int  Run_ccm(stISPParams* gISPparam, u16 *Rgbdata)
{
	int H = gISPparam->rawinfo.H;
	int W = gISPparam->rawinfo.W;
	int imagesize = H * W;
	double(* sccm)[3] = gISPparam->ccm_Param.sccm;
	double r = 0;
	double g = 0;
	double b = 0;
	int maxvalue = (1<<gISPparam->ccm_Param.ccmbit)-1;
	for (int i=0;i<H;i++)
	{
		for (int j=0;j<W;j++)
		{
			r = Rgbdata[i*W+j+2*imagesize];
			g = Rgbdata[i * W + j];
			b = Rgbdata[i * W + j + 1 * imagesize];
			Rgbdata[i * W + j + 2 * imagesize] =(u16)CLIP(r*sccm[0][0]+g*sccm[0][1]+b*sccm[0][2],0, maxvalue);
			Rgbdata[i * W + j] = (u16)CLIP(r * sccm[1][0] + g * sccm[1][1] + b * sccm[1][2],0,maxvalue);
			Rgbdata[i * W + j + 1 * imagesize] = (u16)CLIP( r * sccm[2][0] + g * sccm[2][1] + b * sccm[2][2],0,maxvalue);
		}
	}


	if (gISPparam->rawinfo.ispmode == ISPCombined)
	{
		debug_write16bit_image(H, W, Rgbdata, "2-ccm.jpg");
	}
	else
	{
		debug_write12bit_image(H, W, Rgbdata, "2-ccm.jpg");
	}


	

	return 0;
}