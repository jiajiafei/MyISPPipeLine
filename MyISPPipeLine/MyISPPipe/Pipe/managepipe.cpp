#include"mangepipe.h"
#include<iostream>
#include <stdio.h>
#include <time.h>
u16* Rawdata = NULL;
u16* RawdataBck = NULL;
u32* decompanddata = NULL;
u16* Rgbdata = NULL;
u8* Rgbdata8bit = NULL;
u16* data_extern = NULL;
stISPParams* gISPparam = NULL;
static int firstRun = 0;
eISPMode g_current_mode = ISPCombined;
 char current_raw_path[260] = "example.raw"; // 存储选中的路径
// 修改函数签名，增加 path 参数
int Readraw(u16* rawdata, stRawInfo* rawinfo, const char* path) {
	int H = rawinfo->H;
	int W = rawinfo->W;
	FILE* fraw = NULL;

	// 使用传入的 path 替换硬编码路径
	errno_t ret = fopen_s(&fraw, path, "rb");
	if (ret != 0) {
		printf("Read file error: %s\n", path);
		return -1;
	}

	fread(rawdata, sizeof(u16), H * W, fraw);
	fclose(fraw);
	return 0;
}
int PipeInit()
{
	
	stRawInfo* rawinfo = getRawInfo();
	rawinfo->ispmode = g_current_mode;
	printf("init mode=%d\n", g_current_mode);

	printf("rawinfo mode=%d\n", rawinfo->ispmode);

	if (rawinfo == NULL)
	{
		printf("malloc error\n");
		return -1;
	}
	if (current_raw_path[0] == '\0') {
		printf("No RAW file selected!\n");
		return -2;
	}
	if (firstRun == 0)
	{
		Rawdata = (u16*)malloc(sizeof(u16) * rawinfo->H * rawinfo->W);
		RawdataBck = (u16*)malloc(sizeof(u16) * rawinfo->H * rawinfo->W);
		Readraw(Rawdata, rawinfo, current_raw_path);
		memcpy(RawdataBck, Rawdata, sizeof(u16) * rawinfo->H * rawinfo->W);
		firstRun++;
	}
	else
	{
		memset(Rawdata,0,sizeof(u16)* rawinfo->H * rawinfo->W);
		Readraw(Rawdata, rawinfo, current_raw_path);
		memcpy(RawdataBck, Rawdata, sizeof(u16) * rawinfo->H * rawinfo->W);
	}
	
	gISPparam = getISPParam();
	decompanddata = (u32*)malloc(sizeof(u32) * gISPparam->rawinfo.H * gISPparam->rawinfo.W);
	Rgbdata = (u16*)malloc(sizeof(u16) * gISPparam->rawinfo.H * gISPparam->rawinfo.W * 3);
	Rgbdata8bit = (u8*)malloc(sizeof(u8) * gISPparam->rawinfo.H * gISPparam->rawinfo.W * 3);
	data_extern = (u16*)malloc(sizeof(u16) * (gISPparam->rawinfo.H+ 2*gISPparam->raw_extern.entern_num) \
		* (gISPparam->rawinfo.W + gISPparam->raw_extern.entern_num*2));


	return 0;
};
int PipeRun()
{
	

	if (Rawdata!=NULL&& RawdataBck!=NULL)
	{
		memcpy(Rawdata, RawdataBck,sizeof(u16) * gISPparam->rawinfo.H * gISPparam->rawinfo.W);
	}
	
	int ret = 0;
	if (1 == gISPparam->depwl_param.enable)
	{
		ret = Run_depwl(gISPparam, Rawdata, decompanddata);
		errorinfo(ret,"run depwl!");
	}
	if (1 == gISPparam->dpc_Param.enable)
	{
		ret = DPC_run(gISPparam, decompanddata);
		errorinfo(ret, "run dpc!");
	}
	if (1 == gISPparam->lsc_Param.enable)
	{
		ret = LSC_run(gISPparam, decompanddata);
		errorinfo(ret, "run lsc!");
	}

	if (1 == gISPparam->blc_Param.enable)
	{
		ret = BLC_run(gISPparam, decompanddata);
		errorinfo(ret, "run blc!");
	}
	if (1 == gISPparam->awb_Param.enable)
	{

		ret = AWB_run(gISPparam, decompanddata);
		errorinfo(ret, "run awb!");

	}
	if (1 == gISPparam->tonemap_Param.enable)
	{
		ret = Run_tmp(gISPparam, decompanddata, Rawdata);
		errorinfo(ret, "run tone mapping!");
	}
	if (1 == gISPparam->rnr_Param.enable)
	{
		clock_t start, end;
		double cpu_time_used;
		start = clock();
		ret = Run_RawNR(gISPparam, Rawdata, Rawdata);
		errorinfo(ret, "run RawNR!");
		end = clock();   // 结束计时
		cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

		printf("耗时: %f 秒\n", cpu_time_used);
	}
	if (1 == gISPparam->raw_extern.enable)
	{
		ret=Raw_extern(gISPparam, Rawdata, data_extern);
		
		errorinfo(ret, "run Raw_extern!");
	}
	if (1 == gISPparam->demosaic_Param.enable)
	{
		ret = Run_demosaic(gISPparam, data_extern, Rgbdata);
		errorinfo(ret, "run Run_demosaic!");
	}
	if (1 == gISPparam->ccm_Param.enable)
	{
		ret = Run_ccm(gISPparam, Rgbdata);
		errorinfo(ret, "run Run_ccm!");
	}
	if (1 == gISPparam->gamma_Param.enable)
	{
		ret = Run_sgamma(gISPparam, Rgbdata);
		errorinfo(ret, "run Run_gamma!");
	}
	if (1 == gISPparam->contrast_Param.enable)
	{
		ret = Run_contrast(gISPparam, Rgbdata, Rgbdata8bit);
		errorinfo(ret, "run Run_contrast!");
	}
		if (1 == gISPparam->saturation_Param.enable)
	{
		clock_t start, end;
		double cpu_time_used;
		start = clock();
		ret = Run_saturation(gISPparam, Rgbdata8bit, Rgbdata8bit);
		errorinfo(ret, "run Run_saturation!");
		end = clock();   // 结束计时
		cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

		printf("times: %f 秒\n", cpu_time_used);
	}
	if (1 == gISPparam->sharpen_Param.enable)
	{
		ret = Run_sharpen(gISPparam, Rgbdata8bit, Rgbdata8bit);
		errorinfo(ret, "run Run_sharpen!");
	}
	printf("process is success!\n");

	return 0;
};
