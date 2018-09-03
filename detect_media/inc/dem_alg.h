/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : dem_alg.h
Version       : V1.0
Author        :
Created       : 2018/08/29
Last Modified :
Description   :
Function List :
History       :
 1.Date        : 2018/08/29
   Author      : kapoo pai
   Modification: Created file

******************************************************************************/

#ifndef __DEM_ALG_H__
#define __DEM_ALG_H__

typedef enum ALG_PLAT
{
    PLAT_VPU,                           //alg plat is vpu(Movidius Myraid2)
    PLAT_CPU,

    PLAT_UNSUPPORT
}DEM_ALG_PLAT;

typedef enum ALG_TYPE
{
    TYPE_OBJDETECT,

    TYPE_UNSUPPORT
}DEM_ALG_TYPE;

typedef enum ALG_GRAP
{
    GRAP_YOLOV2,                        // alg graph Yolo V2

    GRAP_UNSUPPORT
}DEM_ALG_GRAP;

typedef struct ALG_HANDLE
{
    DEM_ALG_PLAT plat;                      // plat type for alg
    DEM_ALG_TYPE type;                      // alg type
    DEM_ALG_GRAP graph;                     // alg graph type

    void *plat_handle;
    void *(*alg_run)(char *input_image, int image_len, char *graph_name);     // alg running
}DEM_ALG_HANDLE;

/*******************************************************************************
 * name    : DEM_alg_module_init
 * function: module initlizatin.
 * input   :
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int DEM_alg_module_init(void);

#endif /* __DEM_ALG_H__ */

