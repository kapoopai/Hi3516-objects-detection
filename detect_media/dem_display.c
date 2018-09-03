/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : dem_display.c
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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>


#include "dem_globals.h"
#include "dem_config.h"
#include "dem_display.h"
#include "dem_hi_wrap.h"

#define OBJ_BOXES_ELEMENTSNUM           (8)
#define MAX_BOXES_NUM                   (10)

#define DATA_MEMSIZE                    (100000)

typedef struct DISPLAY_HANDLE
{
    int init_flag;

    pthread_t thr_id;          // thread id
    pthread_mutex_t mutex;      // thread mutex

    char *data_addr;             // address for save capture picture
    DEM_DISPLAY_TYPE type;		// type
}DEM_DISPLAY_HANDLE;

DEM_DISPLAY_HANDLE g_dis_handle;

#define OBJDETECTS_CLASS_NUM       (21)     // all class number of TINY YOLO V2
//float networkMeanNet[] = {0.40787054*255.0, 0.45752458*255.0, 0.48109378*255.0};
const char *g_objs_class_name[OBJDETECTS_CLASS_NUM] = {
    "bg",
    "aeroplane",
    "bicycle",
    "bird",
    "boat",
    "bottle",
    "bus",
    "car",
    "cat",
    "chair",
    "cow",
    "diningtable",
    "dog",
    "horse",
    "motorbike",
    "person",
    "pottedplant",
    "sheep",
    "sofa",
    "train",
    "tvmonitor"
};

/*******************************************************************************
 * name    : DEM_display_data_set
 * function: set the alg result data for display.
 * input   : data:  input result data
 *           type:  data type
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int DEM_display_data_set(void *data, DEM_DISPLAY_TYPE type)
{
	if (NULL == data)
	{
        printf("[%s: %d]data is NULL!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }

    int num = 0;
    switch (type)
    {
    	case DIS_YOLOV2_OBJDTCS:
    	num = *(int *)data;
    	printf("[%s: %d]box number is: %d!\n", __func__, __LINE__, num);
    	pthread_mutex_lock(&g_dis_handle.mutex);
    	memcpy(g_dis_handle.data_addr, (char *)data, sizeof(int) + sizeof(float) * num * OBJ_BOXES_ELEMENTSNUM);
    	g_dis_handle.type = type;
    	pthread_mutex_unlock(&g_dis_handle.mutex);
    	break;

    	default:
    	return DEM_COMMONFAIL;
    }

    return DEM_SUCCESS;
}

/*******************************************************************************
 * name    : display_task_func
 * function: task function for jdisplay
 * input   :
 * output  :
 * return  :
 *******************************************************************************/
void *display_task_func(void *arg)
{
    prctl(PR_SET_NAME, (unsigned long)"display_task", 0, 0, 0);

    printf("[%s: %d]tread display_task start process!\n", __func__, __LINE__);
    while (DEM_TRUE)
    {
        // printf("[%s: %d]start to get picture!\n", __func__, __LINE__);

        int num = 0;
        char *class_name[num];
        int act_num = 0;
        float act_boxes[num][4];         // X1, Y1, X2, Y2
        // lock
        pthread_mutex_lock(&g_dis_handle.mutex);
        DEM_DISPLAY_TYPE type = g_dis_handle.type;
		switch (type)
		{
            case DIS_YOLOV2_OBJDTCS:
                num = *(int *)g_dis_handle.data_addr;              

                if (0 < num && MAX_BOXES_NUM > num)
                {
                    printf("[%s: %d]box number is: %d!\n", __func__, __LINE__, num);
                    int idx = -1;
                    float *box_addr = (float *)((char *)g_dis_handle.data_addr + sizeof(num));
                    int loop = 0;

                    for (loop = 0; loop < num; ++loop)
                    {
                        box_addr += loop * OBJ_BOXES_ELEMENTSNUM;
                        idx = (int)(*(box_addr + 4));

                        #if 0
                        printf(">>>>> [%s: %d]idx: %d, box info: %f, %f, %f, %f, %f, %f, %f, %f\n", 
                            __func__, __LINE__, idx,
                            *(box_addr    ), *(box_addr + 1), *(box_addr + 2), *(box_addr + 3), 
                            *(box_addr + 4), *(box_addr + 5), *(box_addr + 6), *(box_addr + 7));
                        #endif

                        if (idx >= 0 && idx < OBJDETECTS_CLASS_NUM)
                        {

                            class_name[loop]   = (char *)g_objs_class_name[idx];
                            act_boxes[loop][0] = (*(box_addr))     - (*(box_addr + 2)) / 2.0;
                            act_boxes[loop][1] = (*(box_addr + 1)) - (*(box_addr + 3)) / 2.0;
                            act_boxes[loop][2] = (*(box_addr))     + (*(box_addr + 2)) / 2.0;
                            act_boxes[loop][3] = (*(box_addr + 1)) + (*(box_addr + 3)) / 2.0;

                            act_boxes[loop][0] = act_boxes[loop][0] > 0 ? act_boxes[loop][0] : 0;
                            act_boxes[loop][1] = act_boxes[loop][1] > 0 ? act_boxes[loop][1] : 0;
                            act_boxes[loop][2] = act_boxes[loop][2] < 1 ? act_boxes[loop][2] : 1;
                            act_boxes[loop][3] = act_boxes[loop][3] < 1 ? act_boxes[loop][3] : 1;

                            act_num++;
                        }
                        else
                        {
                            printf("[%s: %d]ERROR!! class idx is overlimit: %d!\n", __func__, __LINE__, idx);
                        }

                        #if 0
                        printf(">>>>> [%s: %d]idx: %d, box info: %f, %f, %f, %f\n", __func__, __LINE__, idx,
                            act_boxes[loop][0], act_boxes[loop][1], act_boxes[loop][2], act_boxes[loop][3]);
                        #endif
                    } 
                }

                // unlock
                pthread_mutex_unlock(&g_dis_handle.mutex);

                // printf("[%s: %d]start to call HI_wrap_label_box_show, box number is: %d!\n", __func__, __LINE__, act_num);
                HI_wrap_label_box_show(act_num, class_name, &act_boxes[0][0]);
            break;

            default:
                // unlock
                pthread_mutex_unlock(&g_dis_handle.mutex);
			break;
		}

        // printf("[%s: %d]finish to get picture!\n", __func__, __LINE__);
        usleep(50000);
    }

    printf("[%s: %d]tread display_task end process!\n", __func__, __LINE__);
}

/*******************************************************************************
 * name    : display_process_start
 * function: start to display.
 * input   : 
 * output  :
 * return  : success: DEM_SUCCESS
 *           fail: error number
 *******************************************************************************/
int display_process_start(void)
{
    pthread_attr_t attr;
    int ret = DEM_COMMONFAIL;

    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x96000);     /* 600K */

    ret = pthread_create(&g_dis_handle.thr_id, &attr, display_task_func, NULL);
    if (DEM_SUCCESS != ret)
    {
        printf("[%s: %d]pthread_create display_task_func fail!\n", __func__, __LINE__);
        return DEM_CTHRFAIL;
    }

    return DEM_SUCCESS;
}

/*******************************************************************************
 * name    : DEM_display_module_init
 * function: module initlizatin.
 * input   :
 * output  :
 * return  : DEM_SUCCESS or errno number
 *******************************************************************************/
int DEM_display_module_init(void)
{
	if (DEM_MODULE_INIT == g_dis_handle.init_flag)
    {
        printf("[%s: %d]display module has initialized!\n", __func__, __LINE__);
        return DEM_SUCCESS;
    }

    DEM_CONFIG_ST *cfg = DEM_cfg_get_global_params();
    if (NULL == cfg)
    {
        printf("[%s: %d]cfg_get_global_configures fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }

    memset(&g_dis_handle, 0, sizeof(g_dis_handle));
    if (NULL == g_dis_handle.data_addr)
    {
    	g_dis_handle.data_addr = (char *)malloc(DATA_MEMSIZE);
    }

    if (NULL == g_dis_handle.data_addr)
    {
        memset(g_dis_handle.data_addr, 0, DATA_MEMSIZE);
    	printf("[%s: %d]g_dis_handle.data_addr malloc fail!\n", __func__, __LINE__);
        return DEM_COMMONFAIL;
    }

    // create mutex lock
    if (DEM_SUCCESS != pthread_mutex_init(&g_dis_handle.mutex, NULL))
    {
        printf("[%s: %d]pthread_mutex_init fail!\n", __func__, __LINE__);
        goto FIRST_FAIL;
    }

    // create thread
    if (DEM_SUCCESS != display_process_start())
    {
        printf("[%s: %d]display_process_start fail!\n", __func__, __LINE__);
        goto SECOND_FAIL;
    }

    g_dis_handle.init_flag = DEM_MODULE_INIT;

    return DEM_SUCCESS;
#if 0
THIRD_FAIL:
	if (0 != g_dis_handle.thr_id)
	{
		pthread_join(g_dis_handle.thr_id, 0);
		g_dis_handle.thr_id = 0;
	}
#endif
SECOND_FAIL:
	pthread_mutex_destroy(&g_dis_handle.mutex);

FIRST_FAIL:
	if (NULL == g_dis_handle.data_addr)
	{
		free(g_dis_handle.data_addr);
		g_dis_handle.data_addr = NULL;
	}

	return DEM_COMMONFAIL;
}