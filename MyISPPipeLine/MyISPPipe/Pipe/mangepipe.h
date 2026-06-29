#ifndef _PIPEMANGE_H_
#define _MANGEPIPE_H_
#include "../parameters/parameters.h"
#include"../module/depwl.h"
#include"../module/blc.h"
#include"../module/DPC.h"
#include "../module/lsc.h"
#include "../module/Tonemap.h"
#include"../module/demosaic.h"
#include"../module/raw_extern.h"
#include"../module/ccm.h"
#include"../module/gamma.h"
#include"../module/contrast.h"
#include"../module/saturation.h"
#include"../module/RNR.h"
#include"../module/sharpen.h"
#include"../module/awb.h"
int Readraw(u16* rawdata, stRawInfo* rawinfo, const char* path);
int PipeInit();
int PipeRun();

#endif
