/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : dem_alg.c
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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "dem_globals.h"
#include "dem_config.h"
#include "dem_alg.h"
#include "dem_jpeg.h"
#include "dem_display.h"

#include "vpu.h"

static int s_alg_init_flag = DEM_MODULE_UNINIT;
DEM_ALG_HANDLE g_alg_handle[MAX_ALG_PRODUCTS];

/*******************************************************************************
 * name    : alg_plat_handle_get
 * function: get the alg platform handle.
 * input   : plat_type:     platform type
 * output  :
 * return  : success: plat handle address
 *           fail:    NULL
 *******************************************************************************/
void *alg_plat_handle_get(DEM_ALG_PLAT plat_type)
{
    switch (plat_type)
    {
        case PLAT_VPU:
            return (void *)vpu_plat_handle_get();

        case PLAT_CPU:
            //break;

        default:
            return NULL;
    }

    return NULL;
}

/*******************************************************************************
 * name    : alg_run_callback_get
 * function: get the alg callback function.
 * input   : plat_type:     platform type
 * output  :
 * return  : success:   callback function address
 *           fail:      NULL
 *******************************************************************************/
void *alg_run_callback_get(DEM_ALG_PLAT plat_type)
{
    switch (plat_type)
    {
        case PLAT_VPU:
            return (void *)vpu_alg_run;

        case PLAT_CPU:
            //break;

        default:
            return NULL;
    }

    return NULL;
}

/*******************************************************************************
 * name    : alg_run_callback_get
 * function: get the alg callback function.
 * input   : graph_type:     graph type
 *           max_len:        max name length
 * output  : graph_name:     graph name
 * return  :
 *******************************************************************************/
void alg_graph_name_get(DEM_ALG_GRAP graph_type, int max_len, char *graph_name)
{
    if (NULL == graph_name)
    {
        return;
    }

    int save_len = 0;
    switch (graph_type)
    {
        case GRAP_YOLOV2:
            save_len = strlen(GRAPH_YOLOV2_NAME) > max_len - 1 ? max_len - 1 : strlen(GRAPH_YOLOV2_NAME);
            strncpy(graph_name, GRAPH_YOLOV2_NAME, save_len);
            break;

        case PLAT_CPU:
            //break;

        default:
            return;
    }
}

/*******************************************************************************
 * name    : alg_handle_get
 * function: get the alg handle.
 * input   : plat_name:     platform name
 *           alg_name:      alg name
 *           graph_name:    alg graph name
 * output  :
 * return  : success:   address of alg handle
 *           fail:      NULL
 *******************************************************************************/
DEM_ALG_HANDLE *alg_handle_get(char *plat_name, char *alg_name, char *graph_name)
{
    if (NULL == plat_name || NULL == alg_name || NULL == graph_name)
    {
        printf("[%s: %d]alg param is null!\n", __func__, __LINE__);
        return NULL;
    }

    printf("[%s: %d]start to get alg handle, plat: %s, alg: %s, graph: %s!\n",
         __func__, __LINE__, plat_name, alg_name, graph_name);


    DEM_ALG_PLAT plat_type = PLAT_UNSUPPORT;
    DEM_ALG_TYPE alg_type = TYPE_UNSUPPORT;
    DEM_ALG_GRAP graph_type = GRAP_UNSUPPORT;

    // get plat type
    if (0 == strcmp(plat_name, VPU_NAME))
    {
        plat_type = PLAT_VPU;
    }
    else if (0 == strcmp(plat_name, CPU_NAME))
    {
        plat_type = PLAT_CPU;
    }

    // get alg type
    if (0 == strcmp(alg_name, ALG_OBJDET_NAME))
    {
        alg_type = TYPE_OBJDETECT;
    }

    // get graph type
    if (0 == strcmp(graph_name, GRAPH_YOLOV2_NAME))
    {
        graph_type = GRAP_YOLOV2;
    }

    // start to find handle
    int handle_id = 0;
    for (handle_id = 0; handle_id < MAX_ALG_PRODUCTS; ++handle_id)
    {
        if (g_alg_handle[handle_id].plat == PLAT_UNSUPPORT)
        {
            // add the new handle
            g_alg_handle[handle_id].plat = plat_type;
            g_alg_handle[handle_id].type = alg_type;
            g_alg_handle[handle_id].graph = graph_type;

            g_alg_handle[handle_id].plat_handle = alg_plat_handle_get(plat_type);
            g_alg_handle[handle_id].alg_run = alg_run_callback_get(plat_type);

            if (NULL == g_alg_handle[handle_id].plat_handle || NULL == g_alg_handle[handle_id].alg_run)
            {
                printf("[%s: %d]fail to get alg plat handle or run function, plat: %s, alg: %s, graph: %s\n",
                    __func__, __LINE__, plat_name, alg_name, graph_name);

                g_alg_handle[handle_id].plat = PLAT_UNSUPPORT;
                g_alg_handle[handle_id].type = TYPE_UNSUPPORT;
                g_alg_handle[handle_id].graph = GRAP_UNSUPPORT;
                g_alg_handle[handle_id].plat_handle = NULL;
                g_alg_handle[handle_id].alg_run = NULL;

                return NULL;
            }

            printf("[%s: %d]success to get alg handle, plat: %d, alg: %d, graph: %d!\n",
                __func__, __LINE__, g_alg_handle[handle_id].plat, g_alg_handle[handle_id].type , g_alg_handle[handle_id].graph);

            return &g_alg_handle[handle_id];
        }
        else if (g_alg_handle[handle_id].plat == plat_type && g_alg_handle[handle_id].type == alg_type)
        {
            // return the handle directly
            printf("[%s: %d]success to get alg handle, plat: %s, alg: %s, graph: %s!\n",
                __func__, __LINE__, plat_name, alg_name, graph_name);
            return &g_alg_handle[handle_id];
        }
    }

    printf("[%s: %d]fail to get alg handle, plat: %s, alg: %s, graph: %s, all handles are used!\n",
             __func__, __LINE__, plat_name, alg_name, graph_name);

    return NULL;
}

/*******************************************************************************
 * name    : alg_task_func
 * function: task function for alg.
 * input   :
 * output  :
 * return  :
 *******************************************************************************/
void *alg_task_func(void *arg)
{
    DEM_ALG_PRAMA_ST *alg_params = (DEM_ALG_PRAMA_ST *)arg;
    if (NULL == arg)
    {
        printf("[%s: %d]thread param is null!\n", __func__, __LINE__);
        return NULL;
    }

    char thread_name[128];

    memset(thread_name, 0, sizeof(thread_name));
    sprintf(thread_name, "alg_%s_%s_%s", alg_params->plat_name, alg_params->alg_name, alg_params->graph_name);
    prctl(PR_SET_NAME, (unsigned long)thread_name, 0, 0, 0);
    printf("[%s: %d]thread_name is: %s!\n", __func__, __LINE__, thread_name);

    DEM_ALG_HANDLE *alg_handle = alg_handle_get(alg_params->plat_name, alg_params->alg_name, alg_params->graph_name);
    if (NULL == alg_handle)
    {
        printf("[%s: %d]alg_handle_get fail!\n", __func__, __LINE__);
        return NULL;
    }

    char *jpeg_buf = NULL;
    void *alg_ret = NULL;
    int jpeg_len = 0;
    char graph_name[MAX_NAME_LEN] = {};

    if (GRAP_YOLOV2 == alg_handle->graph)
    {
        jpeg_buf = malloc(416 * 416 * 3);
    }

    if (NULL == jpeg_buf)
    {
        printf("[%s: %d]malloc jpeg_buf fail!\n", __func__, __LINE__);
        return NULL;
    }

    printf("[%s: %d]thread %s start!\n", __func__, __LINE__, thread_name);
    while (DEM_TRUE)
    {
        if (DEM_SUCCESS != DEM_jpeg_buf_get(jpeg_buf, &jpeg_len))
        {
            usleep(30000);
            continue;
        }

        if (NULL != jpeg_buf && 0 != jpeg_len)
        {
            memset(graph_name, 0, sizeof(graph_name));
            alg_graph_name_get(alg_handle->graph, sizeof(graph_name), graph_name);

            alg_ret = alg_handle->alg_run(jpeg_buf, jpeg_len, graph_name);

            if (NULL == alg_ret)
            {
                printf("[%s: %d]%s: run result is NULL!\n", __func__, __LINE__, thread_name);
            }
            else
            {
                DEM_display_data_set(alg_ret, DIS_YOLOV2_OBJDTCS);
            }
        }

        usleep(100);
        
        alg_ret = NULL;
    }

    free(jpeg_buf);
    printf("[%s: %d]thread %s exit!\n", __func__, __LINE__, thread_name);
}

/*******************************************************************************
 * name    : alg_start_threads
 * function: start alg process thread.
 * input   :
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int alg_start_threads(void)
{
    DEM_CONFIG_ST *cfg = DEM_cfg_get_global_params();
    if (NULL == cfg)
    {
        printf("[%s: %d]cfg_get_global_configures fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }

    int thread_id = 0;
    int ret = DEM_COMMONFAIL;
    for (thread_id = 0; thread_id < MAX_ALG_PRODUCTS; ++thread_id)
    {
        if ((!strcmp(cfg->alg[thread_id].plat_name, "")) ||
            (!strcmp(cfg->alg[thread_id].graph_name, "")))
        {
            continue;
        }

        pthread_attr_t attr;
        pthread_t tid;

        /* Initialize and set thread detached attribute */
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x96000);     /* 600K */

        ret = pthread_create(&tid, &attr, alg_task_func, &cfg->alg[thread_id]);

        if (DEM_SUCCESS != ret)
        {
            printf("[%s: %d]pthread_create fail!\n", __func__, __LINE__);
            return DEM_CTHRFAIL;
        }
    }

    return DEM_SUCCESS;
}

/*******************************************************************************
 * name    : DEM_alg_module_init
 * function: module initlizatin.
 * input   :
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int DEM_alg_module_init(void)
{
    if (DEM_MODULE_INIT == s_alg_init_flag)
    {
        printf("[%s: %d]module alg is initialized!\n", __func__, __LINE__);
        return DEM_SUCCESS;
    }

    int handle_id = 0;
    memset(g_alg_handle, 0, sizeof(g_alg_handle));
    for (handle_id = 0; handle_id < MAX_ALG_PRODUCTS; ++handle_id)
    {
        g_alg_handle[handle_id].plat = PLAT_UNSUPPORT;
        g_alg_handle[handle_id].type = TYPE_UNSUPPORT;
        g_alg_handle[handle_id].graph = GRAP_UNSUPPORT;
    }

    int ret = DEM_COMMONFAIL;
    ret = alg_start_threads();
    if (DEM_SUCCESS != ret)
    {
        printf("[%s: %d]alg_start_threads failed! err: %d\n", __func__, __LINE__, ret);
        return ret;
    }

    s_alg_init_flag = DEM_MODULE_INIT;
    printf("[%s: %d]module alg init success!\n", __func__, __LINE__);

    return DEM_SUCCESS;
}
