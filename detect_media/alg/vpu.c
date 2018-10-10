// Copyright 2018 Intel Corporation.
// The source code, information and material ("Material") contained herein is
// owned by Intel Corporation or its suppliers or licensors, and title to such
// Material remains with Intel Corporation or its suppliers or licensors.
// The Material contains proprietary information of Intel or its suppliers and
// licensors. The Material is protected by worldwide copyright laws and treaty
// provisions.
// No part of the Material may be used, copied, reproduced, modified, published,
// uploaded, posted, transmitted, distributed or disclosed in any way without
// Intel's prior express written permission. No license under any patent,
// copyright or other intellectual property rights in the Material is granted to
// or conferred upon you, either expressly, by implication, inducement, estoppel
// or otherwise.
// Any license under such intellectual property rights must be express and
// approved by Intel in writing.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
// #include "stb_image_resize.h"

#include <mvnc.h>

// graph file names - assume the graph file is in the current directory.
//#define GRAPH_FILE_PATH "../res/graph/"
#define GRAPH_FILE_PATH "./"
#define GRAPH_TINYYOLOV2_NAME GRAPH_FILE_PATH "tiny_yolo_v2.graph"

// image file name - assume we are running in this directory: ncsdk/examples/caffe/GoogLeNet/cpp
//#define IMAGE_FILE_NAME "sample_person.jpg"
// #define IMAGE_FILE_NAME "../../data/images/nps_chair.png"

#ifndef bool
typedef enum bool
{
    false = 0,
    true = 1
}bool;
#endif

#define CLASS_NUM       (21)
// add by kapoo for multi stick demo
// #define MULTI_STICK
// add end

// GoogleNet image dimensions, network mean values for each channel in BGR order.
const int networkDimNet = 416;
//float networkMeanNet[] = {0.40787054*255.0, 0.45752458*255.0, 0.48109378*255.0};
const char *class_name[CLASS_NUM] = {
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

#define DETECTION_THRESHOLD (0.40)

#define IOU_THRESHOLD   (0.30)

struct ncDeviceHandle_t *devHandle = NULL;
struct ncGraphHandle_t *graphHandle = NULL;
static char s_graph_name[32] = {};

struct ncTensorDescriptor_t inputTdNet;
struct ncTensorDescriptor_t outputTdNet;

struct ncFifoHandle_t *bufferInNet = NULL;
struct ncFifoHandle_t *bufferOutNet = NULL;

float *g_pic_buf = NULL;
float *g_ref_result = NULL;



// Prototypes
// void *LoadGraphFile(const char *path, unsigned int *length);
// float *LoadImage(const char *path, int reqsize, float *mean);
// end prototypes

float sigmoid(float x)
{
    return (1 / (1 + exp(-x)));
}

float max(float *scores, int num)
{
    float highest_class_score = 0.0;
    int loop = 0;
    for (loop = 0; loop < num; ++loop)
    {
        if (scores[loop] > highest_class_score)
        {
            highest_class_score = scores[loop];
        }
    }

    return highest_class_score;
}

int max_index(float *scores, int num)
{
    int index = 0;
    int loop = 0;
    float highest_class_score = 0.0;
    for (loop = 0; loop < num; ++loop)
    {
        if (scores[loop] > highest_class_score)
        {
            highest_class_score = scores[loop];
            index = loop;
        }
    }

    return index;
}

void softmax(float *output, int num)
{
    if (NULL == output || 0 == num)
    {
        printf("softmax function args error!!\n");
        return;
    }

    float highest_class_score = 0.0;
    float current_score_total = 0.0;
    int loop = 0;

    highest_class_score = max(output, num);

    for (loop = 0; loop < num; ++loop)
    {
        output[loop] = exp(output[loop] - highest_class_score);
    }

    for (loop = 0; loop < num; ++loop)
    {
        current_score_total += output[loop];
    }

    for (loop = 0; loop < num; ++loop)
    {
        output[loop] = output[loop] * 1.0 / current_score_total;
    }
}

float calculate_overlap(float x1, float w1, float x2, float w2)
{
    float left = x1 - w1 / 2.0;
    float right = x2 - w2 / 2.0;

    float box1_coordinate = left > right ? left : right;

    left = x1 + w1 / 2.0;
    right = x2 + w2 / 2.0;
    float box2_coordinate = left < right ? left : right;

    float overlap = box2_coordinate - box1_coordinate;
    return overlap;
}

float calculate_iou(float *box, float *truth)
{
    // calculate the iou intersection over union by first
    // calculating the overlapping height and width
    float width_overlap = calculate_overlap(box[0], box[2], truth[0], truth[2]);
    float height_overlap = calculate_overlap(box[1], box[3], truth[1], truth[3]);
    // no overlap
    if (width_overlap < 0 || height_overlap < 0)
    {
        return 0.0;
    }

    float intersection_area = width_overlap * height_overlap;
    float union_area = box[2] * box[3] + truth[2] * truth[3] - intersection_area;

    return intersection_area / union_area;
}

bool item_in_list(int item, int *dst, int num)
{
    if (dst == NULL || 0 == num)
    {
        return false;
    }

    int loop = 0;
    for (loop = 0; loop < num; ++loop)
    {
        if (*(dst + loop) == item)
            return true;
    }

    return false;
}

int apply_nms(float *boxes, int item_cnt)
{
    if (NULL == boxes || 0 == item_cnt)
    {
        return 0;
    }
    int box_num = 0;

    int box_info_num = 8;
    float tmp_boxes[item_cnt][box_info_num];

    float truth[box_info_num];
    float box[box_info_num];
    float iou = 0.0;

    // sort by box[7] from largest to smallest
    int loop = 0;
    int loop_y = 0;

    float *left_box = NULL;
    float *right_box = NULL;
    float swap_box[8] = {0.0};
#if 0
    float *box_addr = NULL;
    for (loop = 0; loop < item_cnt; ++loop)
    {
        box_addr = (float *)boxes + loop * 8;
        printf("begin sort box: [%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf]\n", *(box_addr), *(box_addr + 1),
            *(box_addr + 2), *(box_addr + 3), *(box_addr + 4), *(box_addr + 5), *(box_addr + 6), *(box_addr + 7));
    }
#endif
    for (loop = 0; loop < item_cnt; ++loop)
    {
        for (loop_y = loop + 1; loop_y < item_cnt; ++loop_y)
        {
            left_box = (float *)boxes + loop * 8;
            right_box = (float *)boxes + loop_y * 8;

            if ((*((float *)left_box + 7)) < (*((float *)right_box + 7)))
            {
                // swap
                memcpy(swap_box, left_box, sizeof(swap_box));
                memcpy(left_box, right_box, sizeof(swap_box));
                memcpy(right_box, swap_box, sizeof(swap_box));
            }
        }
    }
#if 0
    for (loop = 0; loop < item_cnt; ++loop)
    {
        box_addr = (float *)boxes + loop * 8;
        printf("after sort box: [%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf]\n", *(box_addr), *(box_addr + 1),
            *(box_addr + 2), *(box_addr + 3), *(box_addr + 4), *(box_addr + 5), *(box_addr + 6), *(box_addr + 7));
    }
#endif
    // compare the iou for each of the detected objects
    int next_object = 0;
    int index = 0;
    int high_iou_objs[item_cnt];

    for (loop = 0; loop < item_cnt; ++loop)
    {
        if (item_in_list(loop, high_iou_objs, index))
        {
            continue;
        }

        memcpy(truth, boxes + loop * box_info_num, sizeof(truth));

        for (next_object = loop + 1; next_object < item_cnt; ++next_object)
        {
            if (item_in_list(next_object, high_iou_objs, index))
            {
                continue;
            }

            memcpy(box, boxes + next_object * box_info_num, sizeof(box));
            iou = calculate_iou(box, truth);

            if (iou >= IOU_THRESHOLD)
            {
                high_iou_objs[index++] = next_object;
            }
        }
    }

    // filter detected items
    for (loop = 0; loop < item_cnt; ++loop)
    {
        if (!item_in_list(loop, high_iou_objs, index))
        {
            memcpy(&tmp_boxes[box_num][0], boxes + loop * box_info_num, box_info_num * sizeof(float));
            // printf("box: [%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf]\n", tmp_boxes[box_num][0], tmp_boxes[box_num][1],
            //         tmp_boxes[box_num][2], tmp_boxes[box_num][3], tmp_boxes[box_num][4], tmp_boxes[box_num][5],
            //         tmp_boxes[box_num][6], tmp_boxes[box_num][7]);
            box_num++;
        }
    }

    memcpy(boxes, tmp_boxes, box_num * box_info_num * sizeof(float));
    // printf("%s, %d, boxes number is: %d\n", __func__, __LINE__, box_num);

    return box_num;
}

// Reads a graph file from the file system and copies it to a buffer
// that is allocated internally via malloc.
// Param path is a pointer to a null terminate string that must be set to the path to the
//            graph file on disk before calling
// Param length must must point to an integer that will get set to the number of bytes
//              allocated for the buffer
// Returns pointer to the buffer allcoated.
// Note: The caller must free the buffer returned.
void *LoadGraphFile(const char *path, unsigned int *length)
{
    FILE *fp;
    char *buf;

    fp = fopen(path, "rb");
    if(fp == NULL)
        return 0;
    fseek(fp, 0, SEEK_END);
    *length = ftell(fp);
    rewind(fp);
    if(!(buf = (char*) malloc(*length)))
    {
        fclose(fp);
        return 0;
    }
    if(fread(buf, 1, *length, fp) != *length)
    {
        fclose(fp);
        free(buf);
        return 0;
    }
    fclose(fp);
    return buf;
}
#if 0
// Reads an image file from disk (8 bit per channel RGB .jpg or .png or other formats
// supported by stbi_load.)  Resizes it, subtracts the mean from each channel, and then
// converts to an array of half precision floats that is suitable to pass to ncFifoWriteElem.
// The returned array will contain 3 floats for each pixel in the image the first float
// for a pixel is it's the Blue channel value the next is Green and then Red.  The array
// contains the pixel values in row major order.
// Param path is a pointer to a null terminated string that must be set to the path of the
//            to read before calling.
// Param reqsize must be set to the width and height that the image will be resized to.
//               Its assumed width and height are the same size.
// Param mean must be set to point to an array of 3 floating point numbers.  The three
//            numbers are the mean values for the blue, green, and red channels in that order.
//            each B, G, and R value from the image will have this value subtracted from it.
// Returns a pointer to a buffer that is allocated internally via malloc.  this buffer contains
//         the 32 bit float values that can be passed to ncFifoWriteElem().  The returned buffer
//         will contain reqSize*reqSize*3 half floats.
float *LoadImage(const char *path, int reqSize)
{
    int width, height, cp, i;
    unsigned char *img, *imgresized;
    float *imgfp32;
    clock_t step1, step2, step3, end = 0;
    step1 = clock();

    img = stbi_load(path, &width, &height, &cp, 3);
    if(!img)
    {
        printf("Error - the image file %s could not be loaded\n", path);
        return NULL;
    }

    step2 = clock();
    double seconds  =(double)(step2 - step1)/CLOCKS_PER_SEC;
    printf("step1 Use time is: %.8f\n", seconds);

    imgresized = (unsigned char*) malloc(3*reqSize*reqSize);
    if(!imgresized)
    {
        free(img);
        perror("malloc");
        return NULL;
    }
    stbir_resize_uint8(img, width, height, 0, imgresized, reqSize, reqSize, 0, 3);
    free(img);
    imgfp32 = (float*) malloc(sizeof(*imgfp32) * reqSize * reqSize * 3);
    if(!imgfp32)
    {
        free(imgresized);
        perror("malloc");
        return 0;
    }

    step3 = clock();
    seconds  =(double)(step3 - step2)/CLOCKS_PER_SEC;
    printf("step2 Use time is: %.8f\n", seconds);

    for(i = 0; i < reqSize * reqSize * 3; i++)
        imgfp32[i] = imgresized[i];
    free(imgresized);

    for(i = 0; i < reqSize*reqSize; i++)
    {
        float blue, green, red;
        blue = imgfp32[3*i+2];
        green = imgfp32[3*i+1];
        red = imgfp32[3*i+0];

        imgfp32[3*i+0] = red / 255.0;
        imgfp32[3*i+1] = green / 255.0;
        imgfp32[3*i+2] = blue / 255.0;
    }

    end = clock();
    seconds  =(double)(end - step3)/CLOCKS_PER_SEC;
    printf("step3 Use time is: %.8f\n", seconds);

    return imgfp32;
}
#endif

// Opens one NCS device.
// Param deviceIndex is the zero-based index of the device to open
// Param deviceHandle is the address of a device handle that will be set
//                    if opening is successful
// Returns true if works or false if doesn't.
bool OpenOneNCS(int deviceIndex, struct ncDeviceHandle_t **deviceHandle)
{
    ncStatus_t retCode;
    retCode = ncDeviceCreate(deviceIndex, deviceHandle);
    if (retCode != NC_OK)
    {   // failed to get this device's name, maybe none plugged in.
        printf("Error - NCS device at index %d not found\n", deviceIndex);
        return false;
    }

    // Try to open the NCS device via the device name
    retCode = ncDeviceOpen(*deviceHandle);
    if (retCode != NC_OK)
    {   // failed to open the device.
        printf("Error - Could not open NCS device at index %d\n", deviceIndex);
        return false;
    }

    // deviceHandle is ready to use now.
    // Pass it to other NC API calls as needed and close it when finished.
    printf("Successfully opened NCS device at index %d %p!\n", deviceIndex, *deviceHandle);
    return true;
}


// Loads a compiled network graph onto the NCS device.
// Param deviceHandle is the open device handle for the device that will allocate the graph
// Param graphFilename is the name of the compiled network graph file to load on the NCS
// Param graphHandle is the address of the graph handle that will be created internally.
//                   the caller must call mvncDeallocateGraph when done with the handle.
// Returns true if works or false if doesn't.
bool LoadGraphToNCS(struct ncDeviceHandle_t* deviceHandle, const char* graphFilename, struct ncGraphHandle_t** graphHandle)
{
    ncStatus_t retCode;
    int rc = 0;
    unsigned int mem, memMax, length;

    // Read in a graph file
    unsigned int graphFileLen;

    printf("graphFilename is: %s\n", graphFilename);
    void* graphFileBuf = LoadGraphFile(graphFilename, &graphFileLen);

    length = sizeof(unsigned int);
    // Read device memory info
    rc = ncDeviceGetOption(deviceHandle, NC_RO_DEVICE_CURRENT_MEMORY_USED, (void **)&mem, &length);
    rc += ncDeviceGetOption(deviceHandle, NC_RO_DEVICE_MEMORY_SIZE, (void **)&memMax, &length);
    if(rc)
        printf("ncDeviceGetOption failed, rc=%d\n", rc);
    else
        printf("Current memory used on device is %d out of %d\n", mem, memMax);

    // allocate the graph
    retCode = ncGraphCreate("graph", graphHandle);
    if (retCode)
    {
        printf("ncGraphCreate failed, retCode=%d\n", retCode);
        return retCode;
    }

    // Send graph to device
    retCode = ncGraphAllocate(deviceHandle, *graphHandle, graphFileBuf, graphFileLen);
    if (retCode != NC_OK)
    {   // error allocating graph
        printf("Could not allocate graph for file: %s\n", graphFilename);
        printf("Error from ncGraphAllocate is: %d\n", retCode);
        return false;
    }

    // successfully allocated graph.  Now graphHandle is ready to go.
    // use graphHandle for other API calls and call ncGraphDestroy
    // when done with it.
    printf("Successfully allocated graph for %s\n", graphFilename);

    return true;
}

#if 0
// Runs an inference and outputs result to console
// Param graphHandle is the graphHandle from mvncAllocateGraph for the network that
//                   will be used for the inference
// Param imageFileName is the name of the image file that will be used as input for
//                     the neural network for the inference
// Param networkDim is the height and width (assumed to be the same) for images that the
//                     network expects. The image will be resized to this prior to inference.
// Param networkMean is pointer to array of 3 floats that are the mean values for the network
//                   for each color channel, blue, green, and red in that order.
// Returns tru if works or false if doesn't
bool DoInferenceOnImageFile(struct ncGraphHandle_t *graphHandle, struct ncDeviceHandle_t *dev,
                            struct ncFifoHandle_t *bufferIn, struct ncFifoHandle_t *bufferOut,
                            const char* imageFileName, int networkDim)
{
    ncStatus_t retCode;
    struct ncTensorDescriptor_t td;
    struct ncTensorDescriptor_t resultDesc;
    unsigned int length;

    clock_t start, end = 0;
    start = clock();

    // LoadImage will read image from disk, convert channels to floats
    // subtract network mean for each value in each channel.  Then,
    // return pointer to the buffer of 32Bit floats
    float* imageBuf = LoadImage(imageFileName, networkDim);

    // calculate the length of the buffer that contains the floats.
    // 3 channels * width * height * sizeof a 32bit float
    unsigned int lenBuf = 3*networkDim*networkDim*sizeof(*imageBuf);

    // Read descriptor for input tensor
    length = sizeof(struct ncTensorDescriptor_t);
    retCode = ncFifoGetOption(bufferIn, NC_RO_FIFO_GRAPH_TENSOR_DESCRIPTOR, &td, &length);
    if (retCode || length != sizeof(td)){
        printf("ncFifoGetOption failed, retCode=%d\n", retCode);
        return false;
    }

    // Write tensor to input fifo
    retCode = ncFifoWriteElem(bufferIn, imageBuf, &lenBuf, NULL);
    if (retCode != NC_OK)
    {   // error loading tensor
        printf("Error - Could not load tensor\n");
        printf("    ncStatus_t from mvncLoadTensor is: %d\n", retCode);
        return false;
    }

    // Start inference
    retCode = ncGraphQueueInference(graphHandle, &bufferIn, 1, &bufferOut, 1);
    if (retCode)
    {
        free(imageBuf);
        printf("ncGraphQueueInference failed, retCode=%d\n", retCode);
        return false;
    }
    free(imageBuf);

    // the inference has been started, now call ncFifoReadElem() for the
    // inference result
    printf("Successfully loaded the tensor for image %s\n", imageFileName);

    unsigned int outputDataLength;
    length = sizeof(unsigned int);
    retCode = ncFifoGetOption(bufferOut, NC_RO_FIFO_ELEMENT_DATA_SIZE, &outputDataLength, &length);
    if (retCode || length != sizeof(unsigned int)){
        printf("ncFifoGetOption failed, rc=%d\n", retCode);
        exit(-1);
    }
    float* resultData = (float*) malloc(outputDataLength);

    void* userParam;
    // Read output results
    retCode = ncFifoReadElem(bufferOut, (void*) resultData, &outputDataLength, &userParam);
    if (retCode != NC_OK)
    {
        printf("Error - Could not get result for image %s\n", imageFileName);
        printf("    ncStatus_t from ncFifoReadElem is: %d\n", retCode);
        return false;
    }

    end = clock();
    double seconds  =(double)(end - start)/CLOCKS_PER_SEC;
    printf("Use time is: %.8f\n", seconds);


    // Successfully got the result.  The inference result is in the buffer pointed to by resultData
    printf("Successfully got the inference result for image %s\n", imageFileName);

    /*
    a.  First fp16 value holds the number of valid detections = num_valid.
    b.    The next 6 values are unused.
    c. The next (7 * num_valid) values contain the valid detections data
        Each group of 7 values will describe an object/box These 7 values in order.
        The values are:
          0: image_id (always 0)
          1: class_id (this is an index into labels)
          2: score (this is the probability for the class)
          3: box left location within image as number between 0.0 and 1.0
          4: box top location within image as number between 0.0 and 1.0
          5: box right location within image as number between 0.0 and 1.0
          6: box bottom location within image as number between 0.0 and 1.0
    */
    //char *box_list = (char *)resultData[0];
    unsigned int numResults = outputDataLength/sizeof(float);
    float maxResult = 0.0;
    int index = 0;

    /*
    Tiny Yolo V2 uses a 13 x 13 grid with 5 anchor boxes for each grid cell.
    This specific model was trained with the VOC Pascal data set and is comprised of 20 classes
    The 125 results need to be re-organized into 5 chunks of 25 values
    20 classes + 1 score + 4 coordinates = 25 values
    25 values for each of the 5 anchor bounding boxes = 125 values
    */
    int num_classes = CLASS_NUM - 1;        // firt class is bg
    int num_grids = 13;
    int num_anchor_boxes = 5;

    int row = 0;
    int col = 0;
    int b_box_voltron = 0;

    float reordered_results[num_grids * num_grids][5][25] = {0.0};
    int b_box = 0;
    int b_box_num = 0;
    int b_box_info = 0;
    float *result_addr = resultData;
    for (row = 0; row < num_grids; ++row)
    {
        for (col = 0; col < num_grids; ++col)
        {
            for (b_box_voltron = 0; b_box_voltron < 125; ++ b_box_voltron)
            {

                b_box = row * num_grids + col;
                b_box_num = int(b_box_voltron / 25);
                b_box_info = b_box_voltron % 25;
                reordered_results[b_box][b_box_num][b_box_info] = *((float *)result_addr++);
                // (float)*((float *)resultData + (row * num_grids * 125 + col * 125 + b_box_voltron));
            }
        }
    }

    // shapes for the 5 Tiny Yolo v2 bounding boxes
    float anchor_boxes[] = {1.08, 1.19, 3.42, 4.41, 6.63, 11.38, 9.42, 5.11, 16.62, 10.52};

    // iterate through the grids and anchor boxes and filter out all scores
    // which do not exceed the DETECTION_THRESHOLD
    int anchor_box_num = 0;
    float box_x = 0.0;
    float box_y = 0.0;
    float box_w = 0.0;
    float box_h = 0.0;
    float object_confidence = 0.0;
    float highest_class_score = 0.0;
    float class_list[num_classes] = {0.0};
    float final_object_score = 0.0;

    int class_enum = 0;
    int class_w_highest_score = 0;

    float box[8] = {0.0};
    float boxes[num_grids * num_grids * num_anchor_boxes][8] = {0.0};
    int box_idx = 0;

    for (row = 0; row < num_grids; ++row)
    {
        for (col = 0; col < num_grids; ++col)
        {
            for (anchor_box_num = 0; anchor_box_num < num_anchor_boxes; ++anchor_box_num)
            {
                // calculate the coordinates for the current anchor box
                box_x = (col + sigmoid(reordered_results[row * num_grids + col][anchor_box_num][0])) / 13.0;
                box_y = (row + sigmoid(reordered_results[row * num_grids + col][anchor_box_num][1])) / 13.0;
                box_w = (exp(reordered_results[row * num_grids + col][anchor_box_num][2]) *
                     anchor_boxes[2 * anchor_box_num]) / 13.0;
                box_h = (exp(reordered_results[row * num_grids + col][anchor_box_num][3]) *
                     anchor_boxes[2 * anchor_box_num + 1]) / 13.0;


                // find the class with the highest score
                for (class_enum = 0;  class_enum < num_classes; ++class_enum)
                {
                    class_list[class_enum] = (reordered_results[row * 13 + col][anchor_box_num][5 + class_enum]);
                }

                // perform a Softmax on the classes
                softmax(class_list, num_classes);

                // probability that the current anchor box contains an item
                object_confidence = sigmoid(reordered_results[row * 13 + col][anchor_box_num][4]);

                // highest class score detected for the object in the current anchor box
                highest_class_score = max(class_list, num_classes);

                // index of the class with the highest score
                class_w_highest_score = max_index(class_list, num_classes) + 1;

                // the final score for the detected object
                final_object_score = object_confidence * highest_class_score;

                memset(box, 0.0, sizeof(box));
                box[0] = box_x;
                box[1] = box_y;
                box[2] = box_w;
                box[3] = box_h;
                box[4] = class_w_highest_score;
                box[5] = object_confidence;
                box[6] = highest_class_score;
                box[7] = final_object_score;

                // printf("box: [%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf]\n",
                        // box[0], box[1], box[2], box[3], box[4], box[5], box[6], box[7]);

                // filter out all detected objects with a score less than the threshold
                if (final_object_score > DETECTION_THRESHOLD)
                {
                    memcpy(&boxes[box_idx][0], box, sizeof(box));
                    box_idx++;
                }
            }
        }
    }

    // gets rid of all duplicate boxes using non-maximal suppression
    int box_num = apply_nms((float *)boxes, box_idx);

    if (0 == box_num)
    {
        printf(">>> No Object has been detected!!\n");

        free(resultData);
        return true;
    }

    int image_width = networkDimNet;        //original_img.shape[1]
    int image_height = networkDimNet;        //original_img.shape[0]

    // calculate the actual box coordinates in relation to the input image
    int box_loop = 0;

    float box_xmin = 0.0;
    float box_xmax = 0.0;
    float box_ymin = 0.0;
    float box_ymax = 0.0;

    for (box_loop = 0; box_loop < box_num; ++box_loop)
    {
        memcpy(box, &boxes[box_loop][0], sizeof(box));

        //printf("after copye box: [%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf]\n",
                                //box[0], box[1], box[2], box[3], box[4], box[5], box[6], box[7]);

        box_xmin = (box[0] - box[2] / 2.0) * image_width;
        box_xmax = (box[0] + box[2] / 2.0) * image_width;
        box_ymin = (box[1] - box[3] / 2.0) * image_height;
        box_ymax = (box[1] + box[3] / 2.0) * image_height;
        // ensure the box is not drawn out of the window resolution
        if (box_xmin < 0)
            box_xmin = 0;
        if (box_xmax > image_width)
            box_xmax = image_width;
        if (box_ymin < 0)
            box_ymin = 0;
        if (box_ymax > image_height)
            box_ymax = image_height;

        printf("class: %s, coordinates: (%f, %f) - (%f, %f)\n", class_name[int(box[4])], box_xmin, box_ymin, box_xmax, box_ymax);

        // label shape and colorization
        /*
        label_text = label_name[box[4]] + " " + str("{0:.2f}".format(box[5]*box[6]))
        label_background_color = (70, 120, 70) # grayish green background for text
        label_text_color = (255, 255, 0)   # white text

        label_size = cv2.getTextSize(label_text, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)[0]
        label_left = int(box_xmin)
        label_top = int(box_ymin) - label_size[1]
        label_right = label_left + label_size[0]
        label_bottom = label_top + label_size[1]
        */
    }

    free(resultData);

    return true;
}

// Main entry point for the program
int main(int argc, char** argv)
{
    int rc = 0;
    unsigned int length;

    int loglevel = 2;
    ncStatus_t retCode = ncGlobalSetOption(NC_RW_LOG_LEVEL, &loglevel, sizeof(loglevel));

    if (!OpenOneNCS(0, &devHandle))
    {   // couldn't open first NCS device
        exit(-1);
    }

    if (!LoadGraphToNCS(devHandle, GRAPH_FILE_NAME, &graphHandle))
    {
        ncDeviceClose(devHandle);
        exit(-2);
    }

    // Read tensor descriptors net graph
    struct ncTensorDescriptor_t inputTdNet;
    struct ncTensorDescriptor_t outputTdNet;
    length = sizeof(struct ncTensorDescriptor_t);
    ncGraphGetOption(graphHandle, NC_RO_GRAPH_INPUT_TENSOR_DESCRIPTORS, &inputTdNet,  &length);
    ncGraphGetOption(graphHandle, NC_RO_GRAPH_OUTPUT_TENSOR_DESCRIPTORS, &outputTdNet,  &length);

    // Init & Create Fifos for net
    struct ncFifoHandle_t * bufferInNet;
    struct ncFifoHandle_t * bufferOutNet;
    rc = ncFifoCreate("fifoIn0", NC_FIFO_HOST_WO, &bufferInNet);
    rc += ncFifoAllocate(bufferInNet, devHandle, &inputTdNet, 2);
    rc += ncFifoCreate("fifoOut0", NC_FIFO_HOST_RO, &bufferOutNet);
    rc += ncFifoAllocate(bufferOutNet, devHandle, &outputTdNet, 2);
    if(rc)
        printf("Fifo allocation failed for Googlenet, rc=%d\n", rc);

    printf("\n--- NCS 1 inference ---\n");
    DoInferenceOnImageFile(graphHandle, devHandle, bufferInNet, bufferOutNet, IMAGE_FILE_NAME, networkDimNet, networkMeanNet);
    printf("-----------------------\n");

    retCode = ncFifoDestroy(&bufferInNet);
    retCode = ncFifoDestroy(&bufferOutNet);

    retCode = ncGraphDestroy(&graphHandle);
    graphHandle = NULL;

    retCode = ncDeviceClose(devHandle);
    devHandle = NULL;

}
#endif
/*******************************************************************************
 * name    : vpu_load_image
 * function: load image and save RGB value to buffer
 * input   : image_buf:  image buffer
 *           image_len:  image length
 *           picture_dim: picture width and height, here is 416
 * output  :
 * return  : success: 0,
 *           fail : error number
 *******************************************************************************/
int vpu_load_image(char *image_buf, int image_len, int picture_dim)
#if 0
{
    int width, height, cp, i;
    unsigned char *img;
    
    clock_t start, end = 0;
    start = clock();

    // load file from memory
    img = stbi_load_from_memory((const stbi_uc *)image_buf, image_len, &width, &height, &cp, 3);
    if (NULL == img)
    {
        printf("Error - the image file could not be loaded\n");
        return -1;
    }

    end = clock();
    double seconds  = (double)(end - start)/CLOCKS_PER_SEC;
    printf("[%s: %d]stbi_load_from_memory Use time is: %.8f\n", __func__, __LINE__, seconds);

    unsigned int picture_size = picture_dim * picture_dim * 3;
    // for (i = 0; i < picture_size; i++)
    // {
    //     g_pic_buf[i] = img[i];
    // }

    picture_size = picture_dim * picture_dim;
    for (i = 0; i < picture_size; i++)
    {
        float blue, green, red;
        int idx = 3 * i;
        red   = img[idx];
        green = img[idx + 1];
        blue  = img[idx + 2];

        g_pic_buf[3*i+0] = red / 255.0;
        g_pic_buf[3*i+1] = green / 255.0;
        g_pic_buf[3*i+2] = blue / 255.0;
    }

    return 0;
}
#else
// image input is RGB
{
    int i = 0;
    int picture_size = image_len / 3;

    // clock_t start, end = 0;
    // start = clock();

    for (i = 0; i < picture_size; i++)
    {
        float blue, green, red;
        int idx = 3 * i;
        blue  = image_buf[idx];
        green = image_buf[idx + 1];
        red   = image_buf[idx + 2];

        g_pic_buf[3*i+0] = red / 255.0;
        g_pic_buf[3*i+1] = green / 255.0;
        g_pic_buf[3*i+2] = blue / 255.0;
    }

    // end = clock();
    // double seconds = (double)(end - start)/CLOCKS_PER_SEC;
    // printf("[%s: %d]normaliza Use time is: %.8f\n", __func__, __LINE__, seconds);

    return 0;
}
#endif

/*******************************************************************************
 * name    : vps_parse_result
 * function: start to parse the result.
 * input   : result:  result buffer
 *           outputDataLength: result length
 * output  :
 * return  : success 0
 *******************************************************************************/
int vps_parse_result(float *result, int result_len)
{
    /*
    a.  First fp16 value holds the number of valid detections = num_valid.
    b.    The next 6 values are unused.
    c. The next (7 * num_valid) values contain the valid detections data
        Each group of 7 values will describe an object/box These 7 values in order.
        The values are:
          0: image_id (always 0)
          1: class_id (this is an index into labels)
          2: score (this is the probability for the class)
          3: box left location within image as number between 0.0 and 1.0
          4: box top location within image as number between 0.0 and 1.0
          5: box right location within image as number between 0.0 and 1.0
          6: box bottom location within image as number between 0.0 and 1.0
    */
    //char *box_list = (char *)resultData[0];
    //unsigned int numResults = result_len/sizeof(float);
    //float maxResult = 0.0;
    //int index = 0;

    /*
    Tiny Yolo V2 uses a 13 x 13 grid with 5 anchor boxes for each grid cell.
    This specific model was trained with the VOC Pascal data set and is comprised of 20 classes
    The 125 results need to be re-organized into 5 chunks of 25 values
    20 classes + 1 score + 4 coordinates = 25 values
    25 values for each of the 5 anchor bounding boxes = 125 values
    */
    int num_classes = CLASS_NUM - 1;        // firt class is bg
    int num_grids = 13;
    int num_anchor_boxes = 5;

    int row = 0;
    int col = 0;
    int b_box_voltron = 0;

    float reordered_results[num_grids * num_grids][5][25];
    int b_box = 0;
    int b_box_num = 0;
    int b_box_info = 0;
    float *result_addr = g_ref_result;
    for (row = 0; row < num_grids; ++row)
    {
        for (col = 0; col < num_grids; ++col)
        {
            for (b_box_voltron = 0; b_box_voltron < 125; ++ b_box_voltron)
            {

                b_box = row * num_grids + col;
                b_box_num = (int)(b_box_voltron / 25);
                b_box_info = b_box_voltron % 25;
                reordered_results[b_box][b_box_num][b_box_info] = *((float *)result_addr++);
                // (float)*((float *)resultData + (row * num_grids * 125 + col * 125 + b_box_voltron));
            }
        }
    }

    // shapes for the 5 Tiny Yolo v2 bounding boxes
    float anchor_boxes[] = {1.08, 1.19, 3.42, 4.41, 6.63, 11.38, 9.42, 5.11, 16.62, 10.52};

    // iterate through the grids and anchor boxes and filter out all scores
    // which do not exceed the DETECTION_THRESHOLD
    int anchor_box_num = 0;
    float box_x = 0.0;
    float box_y = 0.0;
    float box_w = 0.0;
    float box_h = 0.0;
    float object_confidence = 0.0;
    float highest_class_score = 0.0;
    float class_list[num_classes];
    float final_object_score = 0.0;

    int class_enum = 0;
    int class_w_highest_score = 0;

    float box[8] = {0.0};
    float boxes[num_grids * num_grids * num_anchor_boxes][8];
    int box_idx = 0;

    for (row = 0; row < num_grids; ++row)
    {
        for (col = 0; col < num_grids; ++col)
        {
            for (anchor_box_num = 0; anchor_box_num < num_anchor_boxes; ++anchor_box_num)
            {
                // calculate the coordinates for the current anchor box
                box_x = (col + sigmoid(reordered_results[row * num_grids + col][anchor_box_num][0])) / 13.0;
                box_y = (row + sigmoid(reordered_results[row * num_grids + col][anchor_box_num][1])) / 13.0;
                box_w = (exp(reordered_results[row * num_grids + col][anchor_box_num][2]) *
                     anchor_boxes[2 * anchor_box_num]) / 13.0;
                box_h = (exp(reordered_results[row * num_grids + col][anchor_box_num][3]) *
                     anchor_boxes[2 * anchor_box_num + 1]) / 13.0;


                // find the class with the highest score
                for (class_enum = 0;  class_enum < num_classes; ++class_enum)
                {
                    class_list[class_enum] = (reordered_results[row * 13 + col][anchor_box_num][5 + class_enum]);
                }

                // perform a Softmax on the classes
                softmax(class_list, num_classes);

                // probability that the current anchor box contains an item
                object_confidence = sigmoid(reordered_results[row * 13 + col][anchor_box_num][4]);

                // highest class score detected for the object in the current anchor box
                highest_class_score = max(class_list, num_classes);

                // index of the class with the highest score
                class_w_highest_score = max_index(class_list, num_classes) + 1;

                // the final score for the detected object
                final_object_score = object_confidence * highest_class_score;

                memset(box, 0.0, sizeof(box));
                box[0] = box_x;
                box[1] = box_y;
                box[2] = box_w;
                box[3] = box_h;
                box[4] = class_w_highest_score;
                box[5] = object_confidence;
                box[6] = highest_class_score;
                box[7] = final_object_score;

                // printf("box: [%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf]\n",
                        // box[0], box[1], box[2], box[3], box[4], box[5], box[6], box[7]);

                // filter out all detected objects with a score less than the threshold
                if (final_object_score > DETECTION_THRESHOLD)
                {
                    memcpy(&boxes[box_idx][0], box, sizeof(box));
                    box_idx++;
                }
            }
        }
    }

    // gets rid of all duplicate boxes using non-maximal suppression
    int box_num = apply_nms((float *)boxes, box_idx);

    if (0 == box_num)
    {
        printf(">>> No Object has been detected!!\n");
        goto SAVE_RESULT;
    }

    int image_width = networkDimNet;        //original_img.shape[1]
    int image_height = networkDimNet;        //original_img.shape[0]

    // calculate the actual box coordinates in relation to the input image
    int box_loop = 0;

    float box_xmin = 0.0;
    float box_xmax = 0.0;
    float box_ymin = 0.0;
    float box_ymax = 0.0;

    for (box_loop = 0; box_loop < box_num; ++box_loop)
    {
        memcpy(box, &boxes[box_loop][0], sizeof(box));

        // printf("after copye box: [%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf]\n",
        //         box[0], box[1], box[2], box[3], box[4], box[5], box[6], box[7]);

        box_xmin = (box[0] - box[2] / 2.0) * image_width;
        box_xmax = (box[0] + box[2] / 2.0) * image_width;
        box_ymin = (box[1] - box[3] / 2.0) * image_height;
        box_ymax = (box[1] + box[3] / 2.0) * image_height;
        // ensure the box is not drawn out of the window resolution
        if (box_xmin < 0)
            box_xmin = 0;
        if (box_xmax > image_width)
            box_xmax = image_width;
        if (box_ymin < 0)
            box_ymin = 0;
        if (box_ymax > image_height)
            box_ymax = image_height;

        printf("class: %s, coordinates: (%f, %f) - (%f, %f)\n", class_name[(int)box[4]], box_xmin, box_ymin, box_xmax, box_ymax);

        // label shape and colorization
        /*
        label_text = label_name[box[4]] + " " + str("{0:.2f}".format(box[5]*box[6]))
        label_background_color = (70, 120, 70) # grayish green background for text
        label_text_color = (255, 255, 0)   # white text

        label_size = cv2.getTextSize(label_text, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)[0]
        label_left = int(box_xmin)
        label_top = int(box_ymin) - label_size[1]
        label_right = label_left + label_size[0]
        label_bottom = label_top + label_size[1]
        */
    }

SAVE_RESULT:
    // copy boxes to result
    memcpy(result, &box_num, sizeof(box_num));

    if (0 != box_num)
    {
        memcpy((float *)((char *)result + sizeof(box_num)), boxes, sizeof(box) * box_num);
    }
    return 0;
}

// #define TIME_WASTE
/*******************************************************************************
 * name    : vpu_main_inference
 * function: start to inference and return DNN result.
 * input   : image_buf:  image buffer
 *           image_len:  image length
 * output  :
 * return  : NULL or inference result
 *******************************************************************************/
void *vpu_main_inference(char *image_buf, int image_len)
{
    if (NULL == image_buf || 0 == image_len)
    {
        printf("[%s: %d]image buffer is NULL!\n", __func__, __LINE__);
        return NULL;
    }

    #ifdef TIME_WASTE
    clock_t start, end = 0;
    start = clock();
    double seconds = 0.0;
    #endif

    // load image and save it to g_pic_buf
    // almost 80ms
    if (0 != vpu_load_image(image_buf, image_len, networkDimNet))
    {
        return NULL;
    }

    #ifdef TIME_WASTE
    end = clock();
    seconds = (double)(end - start)/CLOCKS_PER_SEC;
    printf("[######]vpu_load_image Use time is: %.8f\n", seconds);
    start = end;
    #endif

    // calculate the length of the buffer that contains the floats.
    // 3 channels * width * height * sizeof a 32bit float
    unsigned int lenBuf = 3 * networkDimNet * networkDimNet * sizeof(float);
    ncStatus_t retCode;
    struct ncTensorDescriptor_t td;

    // Read descriptor for input tensor
    // almost 0ms
    unsigned int length = sizeof(struct ncTensorDescriptor_t);
    retCode = ncFifoGetOption(bufferInNet, NC_RO_FIFO_GRAPH_TENSOR_DESCRIPTOR, &td, &length);
    if (retCode || length != sizeof(td))
    {
        printf("ncFifoGetOption NC_RO_FIFO_GRAPH_TENSOR_DESCRIPTOR failed, retCode=%d\n", retCode);
        return NULL;
    }

    #ifdef TIME_WASTE
    end = clock();
    seconds = (double)(end - start)/CLOCKS_PER_SEC;
    printf("[######]ncFifoGetOption Use time is: %.8f\n", seconds);
    start = end;
    #endif

    // Write tensor to input fifo
    // almost 70~90ms
    retCode = ncFifoWriteElem(bufferInNet, g_pic_buf, &lenBuf, NULL);
    if (retCode != NC_OK)
    {   // error loading tensor
        printf("Error - Could not load tensor\n");
        printf("    ncStatus_t from mvncLoadTensor is: %d\n", retCode);
        return NULL;
    }

    #ifdef TIME_WASTE
    end = clock();
    seconds = (double)(end - start)/CLOCKS_PER_SEC;
    printf("[######]ncFifoWriteElem Use time is: %.8f\n", seconds);
    start = end;
    #endif

    // Start inference
    // almost 10ms
    retCode = ncGraphQueueInference(graphHandle, &bufferInNet, 1, &bufferOutNet, 1);
    if (retCode)
    {
        printf("ncGraphQueueInference failed, retCode=%d\n", retCode);
        return NULL;
    }

    #ifdef TIME_WASTE
    end = clock();
    seconds = (double)(end - start)/CLOCKS_PER_SEC;
    printf("[######]ncGraphQueueInference Use time is: %.8f\n", seconds);
    start = end;
    #endif

    // the inference has been started, now call ncFifoReadElem() for the
    // inference result
    // printf("Successfully loaded the tensor for image\n");

    // almost 0ms
    unsigned int outputDataLength;
    length = sizeof(unsigned int);
    retCode = ncFifoGetOption(bufferOutNet, NC_RO_FIFO_ELEMENT_DATA_SIZE, &outputDataLength, &length);
    if (retCode || length != sizeof(unsigned int))
    {
        printf("ncFifoGetOption NC_RO_FIFO_ELEMENT_DATA_SIZE failed, rc=%d\n", retCode);
        return NULL;
    }

    #ifdef TIME_WASTE
    end = clock();
    seconds = (double)(end - start)/CLOCKS_PER_SEC;
    printf("[######]ncFifoGetOption Use time is: %.8f\n", seconds);
    start = end;
    #endif

    if (NULL == g_ref_result)
    {
        g_ref_result = (float*)malloc(outputDataLength);
    }

    void* userParam;
    // Read output results
    // almost 10ms
    retCode = ncFifoReadElem(bufferOutNet, (void*) g_ref_result, &outputDataLength, &userParam);
    if (retCode != NC_OK)
    {
        printf("Error - Could not get result for image\n");
        printf("    ncStatus_t from ncFifoReadElem is: %d\n", retCode);
        return NULL;
    }

    #ifdef TIME_WASTE
    end = clock();
    seconds  = (double)(end - start)/CLOCKS_PER_SEC;
    printf("[######]ncFifoReadElem Use time is: %.8f\n", seconds);
    #endif


    // Successfully got the result.  The inference result is in the buffer pointed to by g_ref_result
    // printf("Successfully got the inference result for image\n");

    if (0 != vps_parse_result(g_ref_result, outputDataLength))
    {
        return NULL;
    }

    return (void *)g_ref_result;
}

/*******************************************************************************
 * name    : vpu_load_graph
 * function: start to load graph.
 * input   :
 * output  :
 * return  : success or error number
 *******************************************************************************/
int vpu_load_graph(char *graph_name)
{
    if (NULL == graph_name)
    {
        printf("[%s: %d]You cann't load a null graph !\n", __func__, __LINE__);
        return -1;
    }

    // if is new graph handle, first to destory the old one
    if (NULL != graphHandle)
    {
        ncGraphDestroy(&graphHandle);
    }

    ncStatus_t rc;
    char graph_file[128] = {0};
    memset(graph_file, 0, sizeof(graph_file));
    if (0 == strcmp(graph_name, "YOLO_V2"))
    {
        strcpy(graph_file, GRAPH_TINYYOLOV2_NAME);
    }

    if (!LoadGraphToNCS(devHandle, graph_file, &graphHandle))
    {
        return -1;
    }

    // Read tensor descriptors net graph
    unsigned int length = sizeof(struct ncTensorDescriptor_t);
    ncGraphGetOption(graphHandle, NC_RO_GRAPH_INPUT_TENSOR_DESCRIPTORS, &inputTdNet,  &length);
    ncGraphGetOption(graphHandle, NC_RO_GRAPH_OUTPUT_TENSOR_DESCRIPTORS, &outputTdNet,  &length);

    // Init & Create Fifos for net
    rc = ncFifoCreate("fifoIn0", NC_FIFO_HOST_WO, &bufferInNet);
    rc += ncFifoAllocate(bufferInNet, devHandle, &inputTdNet, 2);
    rc += ncFifoCreate("fifoOut0", NC_FIFO_HOST_RO, &bufferOutNet);
    rc += ncFifoAllocate(bufferOutNet, devHandle, &outputTdNet, 2);
    if (rc)
    {
        printf("Fifo allocation failed for net, rc=%d\n", rc);
        return -1;
    }

    // save graph name
    strcpy(s_graph_name, graph_name);
    printf("[%s: %d]success to save graph name: %s !\n", __func__, __LINE__, s_graph_name);

    return 0;
}

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
void *vpu_alg_run(char *image_buf, int buf_len, char *graph_name)
{
    // graph name is empty
    if (0 == strcmp(graph_name, ""))
    {
        if (NULL == graphHandle)
        {
            printf("[%s: %d]graphHandle and graph name all are null !\n", __func__, __LINE__);
            return NULL;
        }
        else
        {
            printf("[%s: %d]graph name empty but graph handle is not NULL !\n", __func__, __LINE__);
            return vpu_main_inference(image_buf, buf_len);
        }
    }
    else
    {
        // graph name of prams is equal old graph name
        if (0 == strcmp(graph_name, s_graph_name))
        {
            return vpu_main_inference(image_buf, buf_len);
        }
        else
        {

            printf("[%s: %d]start to load a new graph, name: %s !\n", __func__, __LINE__, graph_name);
            // reload a new graph
            if (0 == vpu_load_graph(graph_name))
            {
                return vpu_main_inference(image_buf, buf_len);
            }
        }
    }

    return NULL;
}

/*******************************************************************************
 * name    : vpu_plat_handle_get
 * function: get vpu handle.
 * input   :
 * output  :
 * return  : address of vpu handle
 *******************************************************************************/
void *vpu_plat_handle_get(void)
{
    if (NULL != devHandle)
    {
        return (void *)devHandle;
    }

    int loglevel = 2;
    ncGlobalSetOption(NC_RW_LOG_LEVEL, &loglevel, sizeof(loglevel));

    if (!OpenOneNCS(0, &devHandle))
    {   // couldn't open first NCS device
        return NULL;
    }


    // malloc picture buf
    if (NULL == g_pic_buf)
    {
        if (NULL == (g_pic_buf = (float *)malloc(sizeof(float) * networkDimNet * networkDimNet * 3)))
        {
            printf("[%s: %d]malloc picture buffer fail!\n", __func__, __LINE__);
            return NULL;
        }
    }

    return devHandle;
}

/*******************************************************************************
 * name    : vpu_plat_handle_drop
 * function: drop vpu handle.
 * input   :
 * output  :
 * return  : aNULL
 *******************************************************************************/
void vpu_plat_handle_drop(void)
{
    if (NULL != g_pic_buf)
    {
        free(g_pic_buf);
        g_pic_buf = NULL;
    }

    if (NULL != g_ref_result)
    {
        free(g_ref_result);
        g_ref_result = NULL;
    }

    if (NULL != bufferInNet)
    {
        ncFifoDestroy(&bufferInNet);
        bufferInNet = NULL;
    }

    if (NULL != bufferOutNet)
    {
        ncFifoDestroy(&bufferOutNet);
        bufferOutNet = NULL;
    }

    if (NULL != graphHandle)
    {
        ncGraphDestroy(&graphHandle);
        graphHandle = NULL;
    }

    if (NULL != devHandle)
    {
        ncDeviceClose(devHandle);
        devHandle = NULL;
    }
}
