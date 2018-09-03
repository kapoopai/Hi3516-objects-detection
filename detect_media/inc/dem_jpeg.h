/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : dem_jpeg.h
Version       : V1.0
Author        :
Created       : 2018/08/26
Last Modified :
Description   : Operations for jpeg encode
Function List :
History       :
 1.Date        : 2018/08/26
   Author      : kapoo pai
   Modification: Created file

******************************************************************************/

#ifndef __DEM_JPEG_H__
#define __DEM_JPEG_H__

/*******************************************************************************
 * name    : DEM_jpeg_module_init
 * function: JPEG module initlizatin.
 * input   :
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int DEM_jpeg_module_init (void);

/*******************************************************************************
 * name    : DEM_jpeg_buf_get
 * function: get current picture for alg.
 * input   :
 * output  : jpeg_buf: buffer for save picture
 *           jpeg_len: buffer for save picture length
 * return  : success: DEM_SUCCESS
 *           fail: error number
 *******************************************************************************/
int DEM_jpeg_buf_get(char *jpeg_buf, int *jpeg_len);

#endif  /* __DEM_JPEG_H__ */
