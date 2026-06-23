#include"parameters.h"
#include<iostream>
extern eISPMode g_current_mode;
stRawInfo  grawinfo = {
2160,
3840,
BGGR,
16,
ISPCombined
};
//zeekr imx728 pwl
 u32 x_coor[36] = { 0,3132,105740,387380,3818601,16777215,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
 u32 y_coor[36] = { 0,938,1851,2396,3251,4095,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
// CMOS 12bit输入（x坐标）→ 24bit输出（y坐标）映射数组
//static u32 y_coor[9] = { 0, 1613, 1757, 2026, 2147, 2359, 2914, 3308, 4095 };
//static u32 x_coor[9] = { 0, 25802, 44245, 130161, 223306, 657330, 3320370, 9775474, 16773183 };
//x8d
#if 0
static u32 x_coor[34] = {
0,63,127,191,319,575,1087,2111,4159,8255,
16447,32831,65599,131135,262207,524351,1048639,
2097215,4194367,8388671,12582975,14680127,15728703,
16252991,16515135,16646207,16711743,16744511,
16760895,16769087,16773183,16773183,16773183,16773183
};

static u32 y_coor[34] = {
0,0,64,109,187,321,549,941,1613,1757,
1856,2026,2147,2225,2359,2457,2625,
2914,3059,3308,3702,3899,3998,
4047,4071,4084,4090,4093,
4094,4095,4095,4095,4095,4095
};
#endif
//x8b
//static u32 x_coor[34] = { 0,255,767,1791,3839,7935 ,12031 ,16127,20223,24319,32511,40703,48895,57087,65279,81663,98047,114431,130815,163583,196351,261887,392959 ,524031 ,786175 ,1048319 ,1572607 ,2096895 ,3145471 ,4194047 ,8388351 ,12582655 ,16776959 ,16777215 };
//static u32 y_coor[34] = { 0,256,320,384,448,512,576,640,704,768,832,896,960,1024,1088,1152,1216,1280,1344,1472,1600,1856,2112,2368,2624,2880,3136,3392,3520,3648,3776,3904,4032,4095 };
 stISPParams gISPparamHDRcombine = {
	.depwl_param = {
		.enable = 1,
		.x = { 0,3132,105740,387380,3818601,16777215,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
		.y = { 0,938,1851,2396,3251,4095,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
		.neepoint = 6,
		.enSliceraw=0
	},
	.lsc_Param = {
		.enable = 0,
		.CirNum = 128,
		.lscBit = 24
	},
	.blc_Param = {
		.enable = 1,
		.blc_r = 64,
		.blc_g = 64,
		.blc_b = 64,
		.BlcBit = 24,
		.bayer = RGGB
	},
	.dpc_Param = {
		.enable = 1,
		.threshold=800000
	},
		.awb_Param = {
		.enable = 1,
		.r_gain = 300,
		.b_gain = 255,
		.C_w1= -0.433407,
		.C_w2= -0.430483,
		.C_w3= 0.420105,
		.awbbit=24,
		.awbbit_af=24
	},
	.rnr_Param = {
		.enable = 1,
		.h_r=25,
		.h_g=25,
		.h_b=25
		},
	.tonemap_Param = {
		.enable = 1,
		.lamda = 0.3,
		.PWLsize = 6,
		.tauR = 5,
		.P = 0.00005,
		.etaF = 0.3,
		.etaC = 0.3,
		.BPCmax = 0.9,
		.BPCmin = 0.1,
		.noise_sigma = 5,
		.TMPBit = 24,
		.TMPoutbit = 16,
		.shadow_boost=1,
		.brightness_ratio=1.5,
		.contrast_k=1,
		.light_compress=1
	},
	.demosaic_Param = {
		.enable = 1,
		.m_alpha = 0.25,
		.m_HVgap = 100,
		.thr_c = 400,
		.thr_y = 300,
		.false_k = 0,
		.Demosaicbit = 16
	},
	.raw_extern = {
		.enable = 1,
		.entern_num = 2},
	.cscParam = {.enable = 1},
	.ccm_Param = {
		.enable = 0,
		.sccm = {{1.22008, -0.27657, 0.05649},{ -0.745235, 1.760152, -0.014917},{0.031945, -0.113778, 1.145723}},
		.ccmbit = 16

		},
	.gamma_Param = {
		.enable = 1,
		.gammabitin=16,
		.gammabitout=12,
		.gamma_power=1.4
	},
	.contrast_Param = {
		.enable=1,
		.contrastbit=8,
		.ContrastStrength=128
	},
	.saturation_Param = {
		.enable = 1,
		.strength=256
	},
	.ldciParam={.enable = 1},
	.sharpen_Param={
		.enable = 1,
		.sharpenbit=8,
		.gain_fine=1,
		.gain_mid=1,
		.coring_high=2,
		.under_shot_limit =2 ,
	    .over_shot_limit =2 
		},
	.rawinfo= grawinfo
};
stISPParams gISPparamLinear = {
   .depwl_param = {
		.enable = 1,
		.x = {0},
		.y = {0},
		.neepoint = 6
	},
	.lsc_Param = {
		.enable = 0,
		.CirNum = 128,
		.lscBit = 16
	},
	.blc_Param = {
		.enable = 1,
		.blc_r = 64,
		.blc_g = 64,
		.blc_b = 64,
		.BlcBit = 16,
		.bayer = RGGB
	},
	.dpc_Param = {
		.enable = 1,
		.threshold = 800000
	},
	.awb_Param = {
	.enable = 1,
	.r_gain = 285,
	.b_gain = 375,
	.C_w1 = -0.433407,
	.C_w2 = -0.430483,
	.C_w3 = 0.420105,
	.awbbit = 16,
	.awbbit_af = 16
	},
	.rnr_Param = {
		.enable = 0,
		.h_r = 25,
		.h_g = 25,
		.h_b = 25
		},

	.tonemap_Param = {
		.enable = 1,
		.lamda = 1,
		.PWLsize = 6,
		.tauR = 5,
		.P = 0.00005,
		.etaF = 0.5,
		.etaC = 0.5,
		.BPCmax = 0.9,
		.BPCmin = 0.1,
		.noise_sigma = 5,
		.TMPBit = 16,
		.TMPoutbit=12,
		.shadow_boost = 0,
		.brightness_ratio = 1,
		.contrast_k = 1,
		.light_compress = 1
	},
	.demosaic_Param = {
		.enable = 1,
		.m_alpha = 0.25,
		.m_HVgap = 100,
		.thr_c = 400,
		.thr_y = 300,
		.false_k = 0,
		.Demosaicbit=12
	},
	.raw_extern = {
		.enable = 1,
		.entern_num = 2},
	.cscParam = {.enable = 1},
	.ccm_Param = {
		.enable = 0,
		.sccm = {{1.22008, -0.27657, 0.05649},{ -0.745235, 1.760152, -0.014917},{0.031945, -0.113778, 1.145723}},
		.ccmbit = 12

		},
	.gamma_Param = {
		.enable = 1,
		.gammabitin = 12,
		.gammabitout = 10,
		.gamma_power = 2
	},
	.contrast_Param = {
		.enable = 1,
		.contrastbit = 8,
		.ContrastStrength = 128
	},
	.saturation_Param = {
		.enable = 1,
		.strength=256
	},
	.ldciParam = {.enable = 1},
	.sharpen_Param = {
		.enable = 1,
		.sharpenbit = 8,
		.gain_fine = 1,
		.gain_mid = 1,
		.coring_high = 2,
		.under_shot_limit = 2 ,
		.over_shot_limit = 2
		},
	.rawinfo = grawinfo
};


stRawInfo* getRawInfo()
{
	return  &grawinfo;
};

stISPParams* getISPParam()
{
	eISPMode ispmode= grawinfo.ispmode;

	if (ispmode == ISPLinear)
	{
		printf("Linear init!\n");
		return &gISPparamLinear;
		
	}
	else
	{
		printf("HDR Combine init!\n");
		return &gISPparamHDRcombine;
	}
	
}
