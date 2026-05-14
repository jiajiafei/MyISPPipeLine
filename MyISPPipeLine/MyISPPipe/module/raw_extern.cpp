#include"raw_extern.h"
int Raw_extern(stISPParams* gISPparam, u16* indata, u16* outdata)
{
	u8 extern_num = gISPparam->raw_extern.entern_num;
	int H = gISPparam->rawinfo.H;
	int W = gISPparam->rawinfo.W;
	int H_extern =H+ extern_num*2;
	int W_extern = W + extern_num*2;
	for (int i = 0; i < H_extern; i++)
	{
		for (int j = 0; j < W_extern; j++)
		{
			outdata[i * W_extern + j]=0;
		}
	}
	for (int i=0;i<H;i++)
	{
		for (int j=0;j<W;j++)
		{
			outdata[(i+ extern_num)*W_extern+j+ extern_num] = indata[i*W+j];
		}
	}

	return 0;
}