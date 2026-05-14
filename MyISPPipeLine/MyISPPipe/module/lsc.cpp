#include<iostream>
#include"lsc.h"

double GetDist(int i, int j, stCoord* coord)
{
	int temp = (j - coord->x) * (j - coord->x) + (i - coord->y) * (i - coord->y);
	return sqrt(temp);
}



void ApplyGain(stISPParams* gISPparam, u32* u16Rawdata,  double* gaintlb, u32* Gainedout)
{
	stCoord* coord = gISPparam->lsc_Param.ptcoord;
	int CirNum = gISPparam->lsc_Param.CirNum;
	int H = gISPparam->rawinfo.H;
	int W = gISPparam->rawinfo.W;
	int BITmax = 1 << gISPparam->lsc_Param.lscBit;
	double dist1 = (coord->x - 0) * (coord->x - 0) + (coord->y - 0) * (coord->y - 0);
	double dist2 = (coord->x - 0) * (coord->x - 0) + (coord->y - H) * (coord->y - H);
	double dist3 = (coord->x - W) * (coord->x - W) + (coord->y - 0) * (coord->y - 0);
	double dist4 = (coord->x - W) * (coord->x - W) + (coord->y - H) * (coord->y - H);
	double maxDist = max(max(max(dist1, dist2), dist3), dist4);
	maxDist = sqrt(maxDist);
	double step = maxDist / (double)CirNum;
	double Pixdist = 0;
	int k = 0;
	for (int i = 0; i < H; i++)
	{
		for (int j = 0; j < W; j++)
		{
			Pixdist = GetDist(i, j, coord);
			double pixGained = 0;
			if (floor(Pixdist / step) > CirNum - 1)
			{
				std::cout << "find extern point!" << std::endl;
			}

			k = CLIP(floor(Pixdist / step), 0, CirNum - 1);
			double gain = 0.0;
			if (k < CirNum - 1)
			{
				pixGained = ((Pixdist - k * step) * gaintlb[k + 1] + ((k + 1) * step - Pixdist) * gaintlb[k]) / step;
				Gainedout[i * W + j] = (unsigned short)CLIP(u16Rawdata[i * W + j] * pixGained, 0, BITmax - 1);
			}
			else
			{
				double pixGained = gaintlb[k];
				Gainedout[i * W + j] = (unsigned short)CLIP(u16Rawdata[i * W + j] * pixGained, 0, BITmax - 1);
			}
		}
	}

}
int LSC_run(stISPParams* gISPparam, u32* decompanddata)
{

	double* gaintlb = (double*)malloc(sizeof(double) * gISPparam->lsc_Param.CirNum * 4);
	u16 ImageH = gISPparam->rawinfo.H;
	u16 ImageW = gISPparam->rawinfo.W;
	u16 CirNum = gISPparam->lsc_Param.CirNum;
	int ImageSize = ImageH * ImageW;
	u32* u16RawdataOut = (u32*)malloc(ImageSize * sizeof(u32));
	u32* u16RawGained = (u32*)malloc(ImageSize * sizeof(u32));
	ISPParam_lsc lsc_Param= gISPparam->lsc_Param;
	stCoord *coord = lsc_Param.ptcoord;
	float alpha = gISPparam->lsc_Param.alpha;
	for (int i = 0; i < 4; i++)
	{
		ApplyGain(gISPparam, u16RawdataOut + i * ImageSize / 4, gaintlb + CirNum * i, u16RawGained + i * ImageSize / 4);
	}

	for (int i = 0; i < ImageH; i++)
	{
		for (int j = 0; j < ImageW; j++)
		{
			u16RawGained[i * ImageW + j] = u16RawGained[i * ImageW + j] * alpha + (1 - alpha) * u16RawdataOut[i * ImageW + j];
		}
	}

	return 0;
}