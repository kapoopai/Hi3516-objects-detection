/******************************************************************************
Copyright (C)
******************************************************************************
File Name     : main.c
Version       : V1.0
Author        :
Created       : 2018/08/26
Last Modified :
Description   :
Function List :
History       :
 1.Date        : 2018/08/26
   Author      : kapoo pai
   Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "dem_globals.h"
#include "dem_config.h"
#include "dem_jpeg.h"
#include "dem_alg.h"
#include "dem_display.h"

void get_pwd(void)
{
    char path[256];
    memset(path, 0, sizeof(path));

    getcwd(path, sizeof(path));

    printf("!!START!! %s\n", path);

    return;
}

int main(int argc, char **argv)
{
    get_pwd();

    // step 1, config module start
    if (DEM_SUCCESS != DEM_cfg_module_init())
    {
        exit(0);
    }

    // step 2, jpeg encode module start
    if (DEM_SUCCESS != DEM_jpeg_module_init())
    {
        exit(0);
    }

    // step 3, alg module start
    if (DEM_SUCCESS != DEM_alg_module_init())
    {
        exit(0);
    }

    // step 4, process for display
    if (DEM_SUCCESS != DEM_display_module_init())
    {
        exit(0);
    }

    // loop
    printf("[%s: %d]START MAIN LOOP!\n", __func__, __LINE__);;
    while (DEM_TRUE)
    {
        sleep(30);
    }

    return 0;
}
