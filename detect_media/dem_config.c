/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : dem_config.c
Version       : V1.0
Author        :
Created       : 2018/08/26
Last Modified :
Description   : Get and check all config params
Function List :
History       :
 1.Date        : 2018/08/26
   Author      : kapoo pai
   Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <string.h>

#include "dem_globals.h"
#include "dem_config.h"
#include "dem_hi_wrap.h"

DEM_CONFIG_ST g_config;
static int s_cfg_init_flag = DEM_MODULE_UNINIT;

/*******************************************************************************
 * name    : DEM_cfg_get_global_params
 * function: get global configuers.
 * input   :
 * output  :
 * return  : address of configuers
 *******************************************************************************/
DEM_CONFIG_ST *DEM_cfg_get_global_params(void)
{
    return &g_config;
}

/*******************************************************************************
 * name    : cfg_vi_ch_valid
 * function: check vi channel number.
 * input   :
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int cfg_vi_ch_valid(void)
{
    return HI_wrap_vi_ch_check(g_config.vi_ch);
}

/*******************************************************************************
 * name    : DEM_cfg_module_init
 * function: module initlizatin.
 * input   :
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int DEM_cfg_module_init(void)
{
    if (DEM_MODULE_INIT == s_cfg_init_flag)
    {
        printf("module cfg is initialized!\n");
        return DEM_SUCCESS;
    }

    memset(&g_config, 0, sizeof(g_config));

    g_config.vi_dev = 0;
    g_config.vi_ch = 0;
    g_config.jpeg_enc_ch = 2;

    g_config.jpeg_res.width = 416;
    g_config.jpeg_res.height = 416;

    strcpy(g_config.alg[0].plat_name, VPU_NAME);
    strcpy(g_config.alg[0].graph_name, GRAPH_YOLOV2_NAME);
    strcpy(g_config.alg[0].alg_name, ALG_OBJDET_NAME);

    g_config.osd.attach_vpss_id = 3;
    g_config.osd.attach_venc_id = 0;

    // check vi is valid
    if (DEM_SUCCESS != cfg_vi_ch_valid())
    {
        printf("%s, %d, cfg_vi_ch_valid ch: %d fail!\n", __func__, __LINE__, g_config.vi_ch);
        return DEM_COMMONFAIL;
    }

    s_cfg_init_flag = DEM_MODULE_INIT;
    printf("%s, %d, module cfg initialize success!\n", __func__, __LINE__);

    return DEM_SUCCESS;
}
