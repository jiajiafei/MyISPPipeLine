#ifndef _COMMON_H_
#define _COMMON_H_
#include<string>
#define min(x,y) ((x)>(y)?(y):(x))
#define max(x,y) ((x)>(y)?(x):(y))
#define CLIP(x,min,max)    ((x)<(min)?(min):((x)>(max)?(max):(x))) 
#define AWB_scale 256
typedef unsigned short u16;
typedef unsigned char u8;
typedef int u32;
#define RCCG 0
typedef enum tagISPMode
{
	ISPLinear=0,
	ISPCombined
}eISPMode;
typedef enum tagBayerPattern
{
	RGGB = 0,
	BGGR,
	GBRG,
	GRBG,
	CRGC
}eBayerPattern;
typedef struct ISPParam_depwl
{
	u8 enable;
	u32  x[36];
	u32  y[36];
	u8 neepoint;
	u8 enSliceraw;

}ISPParam_depwl;

typedef struct tagcoord
{
	int x;
	int y;
}stCoord;
typedef struct ISPParam_lsc
{
	u8 enable;
	float* gaintlb;
	float alpha;
	u8 CirNum;
	stCoord *ptcoord;

	int lscBit;

}ISPParam_lsc;
typedef struct ISPParam_blc
{
	u8 enable;
	u16 blc_r;
	u16 blc_g;
	u16 blc_b;
	u8 BlcBit;
	eBayerPattern bayer;

}ISPParam_blc;
typedef struct ISPParam_dpc
{
	u8 enable;
	int threshold;


}ISPParam_dpc;
typedef struct ISPParam_awb
{
	u8 enable;
	int r_gain;
	int b_gain;
	float C_w1;
	float C_w2;
	float C_w3;
	u16 awbbit;
	u16 awbbit_af;
}ISPParam_awb;
typedef struct ISPParam_rnr
{
	u8 enable ;
	
	float h_r;
	float h_g;
	float h_b;

}ISPParam_rnr;
typedef struct ISPParam_tonemap
{
	u8 enable ;
	double lamda ;//É«²Ê»Ö¸´
	int PWLsize ;
	double tauR ;
	double P ;
	double etaF ;
	double etaC ;
	double BPCmax ;
	double BPCmin ;
	double noise_sigma ;
	int TMPBit;
	int TMPoutbit;
	float shadow_boost;
	float brightness_ratio;
	float contrast_k;
	float light_compress;
}ISPParam_tonemap;
typedef struct ISPParam_demosaic
{
	eBayerPattern bayerpattern;
	u8 enable ;
	double m_alpha;
	int m_HVgap;
	u16 thr_c;
	u16 thr_y;
	u16 false_k;
	u16 Demosaicbit;

}ISPParam_demosaic;

typedef struct ISPParam_rawextern
{
	u8 enable;
	u8 entern_num;
};




typedef struct ISPParam_csc
{
	u8 enable ;

}ISPParam_csc;
typedef struct ISPParam_ccm
{
	u8 enable ;
	double sccm[3][3];
	u16 ccmbit;

}ISPParam_ccm;
typedef struct ISPParam_gamma
{
	u8 enable ;
	u8 gammabitin;
	u8 gammabitout;
	float gamma_power;
}ISPParam_gamma;
typedef struct ISPParam_contrast
{
	u8 enable;
	u8 contrastbit;
	u8 ContrastStrength;
};
typedef struct ISPParam_ldci
{
	u8 enable ;


}ISPParam_ldci;
typedef struct ISPParam_sharpen
{
	u8 enable ;
	u8 sharpenbit;
	float gain_fine;
	float gain_mid;
	float coring_high;
	int under_shot_limit;
	int over_shot_limit ;
#if 0 lapulasi filter
	u8 layers_num  ;
	u8 N  ;
	float fact  ;
	float sigma  ;
	float discretisation_step;
#endif
}ISPParam_sharpen;

inline u16 GetPix(u16* indata, int W, int x, int y)
{
	return indata[y * W + x];
}

inline u32 GetPix(u32* indata, int W, int y, int x)
{
	return indata[y * W + x];
}
void errorinfo(int ret, const char* s);

void debug_write16bit_image(u16 H, u16 W, u16* rgbdata, const char* s);
void debug_write12bit_image(u16 H, u16 W, u16* rgbdata, const char* s);
void debug_write10bit_image(u16 H, u16 W, u16* rgbdata, const char* s);
void debug_write8bit_image(u16 H, u16 W, u8* rgbdata, const char* s);
void GuidedFilter(double* rawdataLog, double* rawdataLogBase, int W, int H, int r, double eps);
void guidedFilterHighFreq_float(const float* hf, const float* guide, float* out,
	int rows, int cols, int r, float eps);
#endif 
