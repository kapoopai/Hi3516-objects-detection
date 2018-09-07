/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : dem_config.h
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

#ifndef __DEM_CONFIG_H__
#define __DEM_CONFIG_H__

typedef struct RESOLUTION_ST
{
    int width;
    int height;
}DEM_RES_ST;

typedef struct ALG_PRAMA_ST
{
    char plat_name[MAX_NAME_LEN];
    char alg_name[MAX_NAME_LEN];
    char graph_name[MAX_NAME_LEN];
}DEM_ALG_PRAMA_ST;

typedef struct OSD_PRAMA_ST
{
    int attach_vpss_id;         // vpss id for attach RGN
    int attach_venc_id;         // venc id for attach RGN
}DEM_OSD_PRAMA_ST;

typedef struct CONFIG_ST
{
    int vi_dev;             // dev number for vi
    int vi_ch;              // channel number for vi

    int jpeg_enc_ch;        // channel number for jpeg encodec
    DEM_RES_ST jpeg_res;    // jpeg resolution
    DEM_ALG_PRAMA_ST alg[MAX_ALG_PRODUCTS];

    DEM_OSD_PRAMA_ST osd;
}DEM_CONFIG_ST;

/*******************************************************************************
 * name    : DEM_cfg_get_global_params
 * function: get global configuers.
 * input   :
 * output  :
 * return  : address of configuers
 *******************************************************************************/
DEM_CONFIG_ST *DEM_cfg_get_global_params(void);

/*******************************************************************************
 * name    : DEM_cfg_module_init
 * function: module initlizatin.
 * input   :
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int DEM_cfg_module_init(void);

#endif  /* __DEM_CONFIG_H__ */
