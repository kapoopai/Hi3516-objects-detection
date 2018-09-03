/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : dem_display.h
Version       : V1.0
Author        :
Created       : 2018/09/02
Last Modified :
Description   : source file for display alg result
Function List :
History       :
 1.Date        : 2018/09/02
   Author      : kapoo pai
   Modification: Created file

******************************************************************************/

#ifndef __DEM_DISPLAY_H__
#define __DEM_DISPLAY_H__

typedef enum DISPLAY_TYPE
{
	DIS_YOLOV2_OBJDTCS,			// YOLO V2 objects detections result

	DIS_INVALID
}DEM_DISPLAY_TYPE;

/*******************************************************************************
 * name    : DEM_display_module_init
 * function: module initlizatin.
 * input   :
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int DEM_display_module_init(void);

/*******************************************************************************
 * name    : DEM_display_data_set
 * function: set the alg result data for display.
 * input   : data:  input result data
 *           type:  data type
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int DEM_display_data_set(void *data, DEM_DISPLAY_TYPE type);

#endif /* __DEM_DISPLAY_H__ */