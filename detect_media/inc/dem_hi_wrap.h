/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : dem_hi_wrap.h
Version       : V1.0
Author        :
Created       : 2018/08/26
Last Modified :
Description   : Virtual wrap for Hi3516 sdk
Function List :
History       :
 1.Date        : 2018/08/26
   Author      : kapoo pai
   Modification: Created file

******************************************************************************/

#ifndef __DEM_HI_WRAP_H__
#define __DEM_HI_WRAP_H__

#include <pthread.h>

/*******************************************************************************
 * name    : HI_wrap_vi_ch_check
 * function: check vi channel number by vi channel number.
 * input   :
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_vi_ch_check(int channel_num);

/*******************************************************************************
 * name    : HI_wrap_jpeg_encodec_create
 * function: create a jpeg encodec.
 * input   : vi_channel_id: channel id of vi
 *           jpeg_channel_id:  channel id of jpeg encodec
 *           width: encode jpeg picture witdh
 *           height: encode jpeg picture height
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_jpeg_encodec_create(int vi_channel_id, int jpeg_channel_id, int width, int height);

/*******************************************************************************
 * name    : HI_wrap_jpeg_cap_get
 * function: capture a jpeg package.
 * input   : enc_ch: encode channel id for capture
 *           max_buf_len:  max buffer length for save
 *           lock:      lock
 * output  : out_buf:   buffer for save picture
 *           out_len:   buffer length for save picture
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_jpeg_cap_get(int enc_ch, int max_buf_len, pthread_mutex_t *lock, char *out_buf, int *out_len);

/*******************************************************************************
 * name    : HI_wrap_osd_fixtype
 * function: attach osd on venc.
 * input   : enc_ch: encode channel id for capture
 *           max_buf_len:  max buffer length for save
 *           lock:      lock
 * output  : out_buf:   buffer for save picture
 *           out_len:   buffer length for save picture
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_osd_fixtype(void);

/*******************************************************************************
 * name    : HI_wrap_label_box_show
 * function: attach labels and boxes on venc.
 * input   : number:      osd boxes number
 *           label_bmp:   labels bmp
 *           bmp_res:     bmp resolution
 *           boxes:       boxes coordinates, such as X1, Y1, X2, Y2
 * output  : 
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_label_box_show(int number, char **label_bmp, DEM_RES_ST *bmp_res, float *boxes);

/*******************************************************************************
 * name    : HI_wrap_draw_cover_labels_fix
 * function: draw the class labels for test.
 * input   : 
 * output  : 
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int HI_wrap_draw_cover_labels_fix(char *bmp_addr);

#endif  /* __DEM_HI_WRAP_H__ */
