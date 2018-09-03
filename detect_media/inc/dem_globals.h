/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : dem_globals.h
Version       : V1.0
Author        :
Created       : 2018/08/26
Last Modified :
Description   : Define all global marcos\variables for this project
Function List :
History       :
 1.Date        : 2018/08/26
   Author      : kapoo pai
   Modification: Created file

******************************************************************************/

#ifndef __DEM_GLOBALS_H__
#define __DEM_GLOBALS_H__

#define DEM_TRUE            (1)
#define DEM_FALSE           (0)

#define DEM_SUCCESS         (0)
#define DEM_COMMONFAIL      (-1)
#define DEM_VICHNFAIL       (-2)        // vi channel id error
#define DEM_JPEGCHNFAIL     (-3)        // jpeg channel id error
#define DEM_VPSSGRPFAIL     (-4)        // vpss group id error
#define DEM_VPSSCHNFAIL     (-5)        // vpss channel id error
#define DEM_RESFAIL         (-6)        // resolution error
#define DEM_CTHRFAIL        (-7)        // create thread fail

#define DEM_MODULE_INIT     (1)
#define DEM_MODULE_UNINIT   (0)

#define MAX_ALG_PRODUCTS    (1)

#define x86    (0)
#define arm    (1)

#define VPU_NAME            "VPU"
#define CPU_NAME            "CPU"
#define ALG_OBJDET_NAME     "objects_detection"
#define GRAPH_YOLOV2_NAME   "YOLO_V2"

#define MAX_NAME_LEN		(32)


#endif  /* __DEM_GLOBALS_H__ */
