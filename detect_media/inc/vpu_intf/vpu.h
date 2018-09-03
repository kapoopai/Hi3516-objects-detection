/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : vpu.h
Version       : V1.0
Author        :
Created       : 2018/08/26
Last Modified :
Description   : The head file for vpu
Function List :
History       :
 1.Date        : 2018/08/30
   Author      : kapoo pai
   Modification: Created file

******************************************************************************/

#ifndef __VPU_H__
#define __VPU_H__

/*******************************************************************************
 * name    : vpu_plat_handle_get
 * function: get vpu handle.
 * input   :
 * output  :
 * return  : address of configuers
 *******************************************************************************/
void *vpu_plat_handle_get(void);

/*******************************************************************************
 * name    : vpu_alg_run
 * function: vpu inference function.
 * input   : image_buf:         image buffer
 *           buf_len:           image buffer len             
 *           graph_name:        calc graph name, such as YOLO v2
 * output  :
 * return  : success: address of alg run result
 *           fail: NULL
 *******************************************************************************/
void *vpu_alg_run(char *image_buf, int buf_len, char *graph_name);



#endif /* __VPU_H__ */
