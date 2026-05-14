#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_
#include"../common/common.h"


typedef struct tagstRawInfo
{
	u16 H;
	u16 W;
	eBayerPattern BayerType;
	u8 rawbit;
	eISPMode ispmode;
}stRawInfo;
typedef struct tagISPParams
{
	ISPParam_depwl depwl_param;
	ISPParam_lsc lsc_Param;
	ISPParam_blc blc_Param;
	ISPParam_dpc dpc_Param;
	ISPParam_awb awb_Param;
	ISPParam_rnr rnr_Param;
	ISPParam_tonemap tonemap_Param;
	ISPParam_demosaic demosaic_Param;
	ISPParam_rawextern raw_extern;
	ISPParam_csc cscParam;
	ISPParam_ccm ccm_Param;
	ISPParam_gamma gamma_Param;
	ISPParam_contrast contrast_Param;
	ISPParam_ldci ldciParam;
	ISPParam_sharpen sharpen_Param;
	stRawInfo rawinfo;
}stISPParams;

stRawInfo* getRawInfo();
stISPParams* getISPParam();

#endif