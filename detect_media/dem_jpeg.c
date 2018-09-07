/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : dem_jpeg.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "dem_globals.h"
#include "dem_config.h"
#include "dem_jpeg.h"
#include "dem_hi_wrap.h"

#define MAX_CAP_BUFFER      (800000)


typedef struct JPEG_HANDLE
{
    int init_flag;

    pthread_t thr_id;          // thread id
    pthread_mutex_t mutex;      // thread mutex

    char *cap_addr;             // address for save capture picture
    int cap_len;                // lenght
}DEM_JPEG_HANDLE;

char g_capture_pack[MAX_CAP_BUFFER];

DEM_JPEG_HANDLE g_jpeg_handle;

/*******************************************************************************
 * name    : jpeg_capture_task_func
 * function: task function for jpeg capture
 * input   :
 * output  :
 * return  :
 *******************************************************************************/
void *jpeg_capture_task_func(void *arg)
{
    if (NULL == arg)
    {
        printf("[%s: %d]arg error!\n", __func__, __LINE__);
        return NULL;
    }
    int enc_ch = *(int *)arg;
    printf("[%s: %d]get enc channel %d!\n", __func__, __LINE__, enc_ch);

    char thread_name[32];
    memset(thread_name, 0, sizeof(thread_name));
    sprintf(thread_name, "jpeg_capture_ch%d", enc_ch);
    prctl(PR_SET_NAME, (unsigned long)thread_name, 0, 0, 0);

    printf("[%s: %d]thread %s start process!\n", __func__, __LINE__, thread_name);
    while (DEM_TRUE)
    {
        // printf("[%s: %d]start to get picture!\n", __func__, __LINE__);
        HI_wrap_jpeg_cap_get(enc_ch, MAX_CAP_BUFFER, &g_jpeg_handle.mutex, g_jpeg_handle.cap_addr, &g_jpeg_handle.cap_len);
        // printf("[%s: %d]finish to get picture!\n", __func__, __LINE__);
        usleep(50000);
    }

    printf("[%s: %d]tread %s end process!\n", __func__, __LINE__, thread_name);
}


/*******************************************************************************
 * name    : jpeg_capture_start
 * function: start to capture.
 * input   : *enc_ch: encode channel id address for capture
 * output  :
 * return  : success: DEM_SUCCESS
 *           fail: error number
 *******************************************************************************/
int jpeg_capture_start(int *enc_ch)
{
    pthread_attr_t attr;
    int ret = DEM_COMMONFAIL;

    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x96000);     /* 600K */
    printf("[%s: %d]get enc channel %d!\n", __func__, __LINE__, *enc_ch);

    ret = pthread_create(&g_jpeg_handle.thr_id, &attr, jpeg_capture_task_func, enc_ch);
    if (DEM_SUCCESS != ret)
    {
        printf("[%s: %d]pthread_create jpeg_capture_task fail!\n", __func__, __LINE__);
        return DEM_CTHRFAIL;
    }

    return DEM_SUCCESS;
}

/*******************************************************************************
 * name    : jpeg_encodec_create
 * function: create a jpeg encodec.
 * input   : vi_channel_id: channel id of vi
 *           jpeg_channel_id:  channel id of jpeg encodec
 *           resolution: encode jpeg picture resolution
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int jpeg_encodec_create(int vi_channel_id, int jpeg_channel_id, DEM_RES_ST resolution)
{
    if (0 == resolution.width || 0 == resolution.height)
    {
        printf("[%s: %d]jpeg resolution is invalid, width: %d, height: %d!\n", __func__, __LINE__,
            resolution.width, resolution.height);
        return DEM_COMMONFAIL;
    }

    return HI_wrap_jpeg_encodec_create(vi_channel_id, jpeg_channel_id, resolution.width, resolution.height);
}

/*******************************************************************************
 * name    : DEM_jpeg_buf_get
 * function: get current picture for alg.
 * input   :
 * output  : jpeg_buf: buffer for save picture
 *           jpeg_len: buffer for save picture length
 * return  : success: DEM_SUCCESS
 *           fail: error number
 *******************************************************************************/
int DEM_jpeg_buf_get(char *jpeg_buf, int *jpeg_len)
{
    if (NULL == jpeg_buf || NULL == jpeg_len)
    {
        printf("[%s: %d]pointer of jpeg buf or jpeg length is NULL!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }
    int ret = DEM_COMMONFAIL;
    pthread_mutex_lock(&g_jpeg_handle.mutex);

    if (0 != g_jpeg_handle.cap_len)
    {
        memcpy(jpeg_buf, g_jpeg_handle.cap_addr, g_jpeg_handle.cap_len);
        *jpeg_len = g_jpeg_handle.cap_len;

        // printf("[%s: %d]get picture success, len: %d!\n", __func__, __LINE__, *jpeg_len);
        ret = DEM_SUCCESS;
    }
    else
    {
        ret = DEM_COMMONFAIL;
    }
    pthread_mutex_unlock(&g_jpeg_handle.mutex);

    return ret;
}


/*******************************************************************************
 * name    : DEM_jpeg_module_init
 * function: JPEG module initlizatin.
 * input   :
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int DEM_jpeg_module_init(void)
{
    if (DEM_MODULE_INIT == g_jpeg_handle.init_flag)
    {
        printf("[%s: %d]jpeg module has initialized!\n", __func__, __LINE__);
        return DEM_SUCCESS;
    }
    DEM_CONFIG_ST *cfg = DEM_cfg_get_global_params();
    if (NULL == cfg)
    {
        printf("[%s: %d]cfg_get_global_configures fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }

    memset(g_capture_pack, 0, sizeof(g_capture_pack));
    memset(&g_jpeg_handle, 0, sizeof(g_jpeg_handle));
    g_jpeg_handle.cap_addr = g_capture_pack;

    // create mutex lock
    if (DEM_SUCCESS != pthread_mutex_init(&g_jpeg_handle.mutex, NULL))
    {
        printf("[%s: %d]pthread_mutex_init fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }

    // start to bind vi and jpeg encodec channel
    if (DEM_SUCCESS != jpeg_encodec_create(cfg->vi_ch, cfg->jpeg_enc_ch, cfg->jpeg_res))
    {
        printf("[%s: %d]jpeg_encodec_create fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }

    // start jpeg capture task
    printf("[%s: %d]jpeg_capture_start, enc_ch: %d!\n", __func__, __LINE__, cfg->jpeg_enc_ch);
    if (DEM_SUCCESS != jpeg_capture_start(&cfg->jpeg_enc_ch))
    {
        printf("[%s: %d]jpeg_capture_start fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }

    // HI_wrap_osd_fixtype();

    g_jpeg_handle.init_flag = DEM_MODULE_INIT;

    return DEM_SUCCESS;
}
