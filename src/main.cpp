/*******************************************************************************
 Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1   Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 2   Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************/
/**
 ********************************************************************************
 * @file <main.cpp>
 *
 * @brief Main file
 *
 ********************************************************************************
 */
#include "medianFilter.h"
#include "ippMedianFilter.h"
#include "CL/cl.h"
#include "utils.h"
#include "CLUtil.hpp"
#include "SDKBitMap.hpp"
using namespace appsdk;

/******************************************************************************
 * Default parameter values                                                    *
 ******************************************************************************/
#define DEFAULT_FILTER_SIZE             5
#define DEFAULT_INPUT_IMAGE             "Nature_2448x2044.bmp"
#define DEFAULT_OPENCL_OUTPUT_IMAGE     "oclMedianOutput.bmp"
#define DEFAULT_IPP_OUTPUT_IMAGE        "ippMedianOutput.bmp"
#define DEFAULT_BITWIDTH                16

/******************************************************************************
 * Structure to hold the parameters for the sample                             *
 ******************************************************************************/
typedef struct MedianFilter
{
    cl_uint rows;
    cl_uint cols;

    cl_uint paddedRows;
    cl_uint paddedCols;

    cl_uint filterSize;

    cl_uchar *inputImg;
    cl_uchar *oclOutputImg;
    cl_uchar *ippOutputImg;

    cl_kernel medianFilterKernel;
    
    cl_mem input;
    cl_mem output;
    
    Ipp8u* pBuffer;

    SDKBitMap inputBitmap;   /**< Bitmap class object */
} MedianFilter;

/******************************************************************************
 * Number of runs for performance measurement                                  *
 ******************************************************************************/
#define LOOP_COUNT 100

/******************************************************************************
 * Function declaration                                                        *
 ******************************************************************************/
bool readInput(MedianFilter *paramFF, const char *inputImage,
                cl_uint bitWidth);
bool createMemory(MedianFilter* paramFF, DeviceInfo *infoDeviceOcl,
                cl_uint bitWidth);
void destroyMemory(MedianFilter *paramFF, DeviceInfo *infoDeviceOcl);
bool saveOutputs(MedianFilter *paramFF, const char *filename1, const char *filename2,
                cl_uint bitWidth);
bool run(DeviceInfo *infoDeviceOcl, MedianFilter *paramFF,
                cl_uint bitWidth, cl_uint dataTransfer, cl_event *ev);
bool init(DeviceInfo *infoDeviceOcl, MedianFilter *paramFF,
                const char *inputImage, cl_int filterSize,
                cl_uint bitWidth, cl_uint deviceNum, cl_int useLds, cl_uint useIpp);

/**
 *******************************************************************************
 *  @fn     usage
 *  @brief  This function prints the program usage
 *
 *  @param[in] prog : name of the binary to print in the message
 *
 *  @return void
 *******************************************************************************
 */
void usage(const char *prog)
{
    printf("Usage: %s [-i (input image path)]", prog);
    printf("[-bitWidth (8 | 16)][-filtSize (filterSize 3 | 5)][-useLds (0 | 1)]\n");                    
    printf("Example: To run 5X5 filter on 8 bit/channel input image, run");
    printf("\n\t %s -i Nature_2048x1024.bmp -filtSize 5 -bitWidth 8 -useLds 0\n", prog);    
}

int main(int argc, char **argv)
{

    DeviceInfo infoDeviceOcl;
    MedianFilter paramFF;

    cl_uint status = 0;
    cl_int loopCnt = LOOP_COUNT;
    cl_uint bitWidth = DEFAULT_BITWIDTH;
    cl_uint deviceNum = 0;
    cl_int useLds = 0;
    cl_uint useIpp = 0;
    cl_uint datatransfer;
    cl_uint verify = 1;
    
    const char *inputImage = DEFAULT_INPUT_IMAGE;
    const char *medianOutputImage = DEFAULT_OPENCL_OUTPUT_IMAGE;
    const char *ippOutputImage = DEFAULT_IPP_OUTPUT_IMAGE;

    cl_int filterSize = DEFAULT_FILTER_SIZE;

    /***************************************************************************
     * Processing the command line arguments                                   *
     **************************************************************************/
    while (argv[1] && argv[1][0] == '-')
    {
        if (strcmp(argv[1], "-i") == 0)
        {
            argv++;
            argc--;
            inputImage = argv[1];
        }
        else if (strncmp(argv[1], "-filtSize", 9) == 0)
        {
            argv++;
            argc--;
            filterSize = atoi(argv[1]);
            if (!(filterSize == 3 || filterSize == 5))
            {
                printf("Only filter sizes 3 and 5 are supported.\n");
                exit(1);
            }
        }
        else if (strncmp(argv[1], "-bitWidth", 9) == 0)
        {
            argv++;
            argc--;
            bitWidth = atoi(argv[1]);
            if (!(bitWidth == 8 || bitWidth == 16))
            {
                printf("Only 8 and 16 are supported bitWidth.\n");
                exit(1);
            }
        }
        else if (strncmp(argv[1], "-useLds", 8) == 0)
        {
            argv++;
            argc--;
            useLds = atoi(argv[1]);
        }
        else if (strncmp(argv[1], "-device", 7) == 0)
        {
            argv++;
            argc--;
            deviceNum = atoi(argv[1]);
        }
        else if (strncmp(argv[1], "-verify", 7) == 0)
        {
            argv++;
            argc--;
            verify = atoi(argv[1]);
        }
        else
        {
            printf("Illegal option %s ignored\n", argv[1]);
            usage(argv[0]);
            exit(1);
        }
        argv++;
        argc--;
    }

    if (argc > 1)
    {
        usage(argv[0]);
        exit(1);
    }
    
    /***************************************************************************
     * Read input, initialize OpenCL runtime, create memory and OpenCL kernels
     **************************************************************************/
    if (init(&infoDeviceOcl, &paramFF, inputImage, filterSize,
                    bitWidth, deviceNum, useLds, useIpp) != true)
    {
        printf("Error in init.\n");
        return -1;
    }
    
    /***************************************************************************
    * Print information about input and other improtant details              
    **************************************************************************/
    printf("Executing Median filter");
    printf("\n\tFilter size: %dx%d\n\tInput Image: %d bit single channel\n\tInput Image resolution: %dx%d", 
                    filterSize, filterSize, bitWidth, paramFF.cols, paramFF.rows);
    
    if (useLds)
        printf("\n\tKernels are using Lds memory for input.");
    else 
        printf("\n\tKernels are not using Lds memory for input.");

    printf("\n\nRunning for %d iterations\n\n", loopCnt);

    

    /**************************************************************************
     * Run the ipp Median Filter.
     ***************************************************************************/
    cl_double time3;
    timer t_timer3;
    timerStart(&t_timer3);

    for (int i = 0; i < loopCnt; i++)
    {
        runIppMedianFilter(paramFF.inputImg, 
            paramFF.filterSize, 
            paramFF.ippOutputImg, 
            paramFF.cols, 
            paramFF.rows,
            bitWidth,
            paramFF.pBuffer);
    }


    time3 = timerCurrent(&t_timer3);
    time3 = 1000 * (time3 / loopCnt);

    /**************************************************************************
     * OpenCL median Filter.
     ***************************************************************************/
    cl_event *eventList = (cl_event *)malloc(loopCnt * sizeof(cl_event));
    if (!eventList)
    {
        printf("Error mallocing eventList.\n");
        return -1;
    }

    /***************************************************************************
    * Warm-up run of Median filters                            
    **************************************************************************/
    datatransfer = 1;   //Data will be transferred

    if (run(&infoDeviceOcl, &paramFF, bitWidth, datatransfer, NULL) != true)
    {
        printf("Error in run.\n");
        return -1;
    }

    /*******************************************************************************
    * Get Performance data
    *******************************************************************************/
    datatransfer = 0;

    for (int i = 0; i < loopCnt; i++)
    {
        if (run(&infoDeviceOcl, &paramFF, bitWidth, datatransfer, &eventList[i]) != true)
        {
            printf("Error in run.\n");
            return -1;
        }
    }
    clFinish(infoDeviceOcl.mQueue);

    double time_ms = 0;
    for (int i = 0; i < loopCnt; i++)
    {
        cl_ulong time_start, time_end;

        status = clGetEventProfilingInfo(eventList[i], CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
        CHECK_RESULT(status != CL_SUCCESS, "clGetEventProfilingInfo failed. Error code = %d", status);
        
        status = clGetEventProfilingInfo(eventList[i], CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
        CHECK_RESULT(status != CL_SUCCESS, "clGetEventProfilingInfo failed. Error code = %d", status);
        
        time_ms += (double)((time_end - time_start)*(1.0e-6));
        
        clReleaseEvent(eventList[i]);
    }
    free(eventList);

    printf("Average time taken per iteration for OpenCL Median Filter without data transfer is %f msec\n", (time_ms/loopCnt));
    printf("Average time taken for ipp Median Filter is %f msec\n\n", time3);


    /***************************************************************************
    * Save OpenCL and IPP filter output images                  
    **************************************************************************/
    if (!saveOutputs(&paramFF, medianOutputImage, ippOutputImage, bitWidth))
    {
        printf("Error in saveOutputs.\n");
        return -1;
    }

    /***************************************************************************
    * Verify output images                  
    **************************************************************************/
    if (verify)
    {
        if (memcmp(paramFF.oclOutputImg, paramFF.ippOutputImg, paramFF.rows * paramFF.cols * (bitWidth / 8)) != 0)
        {
            printf("\nVerification failed!!\n\n");            
        }
        else
        {
            printf("\nVerification succeeded!!\n\n");            
        }
    }
    
    /***************************************************************************
    * Destpry memory and cleanup OpenCL runtime                              
    **************************************************************************/
    destroyMemory(&paramFF, &infoDeviceOcl);
    
    return 0;
}

/**
 *******************************************************************************
 *  @fn     init
 *  @brief  This function parses creates the memory needed by the pipeline, 
 *          initializes openCL, builds kernel and sets the kernel arguments
 *
 *  @param[in/out] infoDeviceOcl : Structure which holds openCL related params
 *  @param[in/out] paramFF      : Structure holds all parameters required 
 *                                 by the sample
 *  @param[in] inputImage       : input imaage name
 *  @param[in] filterSize       : filter size (only 3 and 5 are currently supported)
 *  @param[in] bitWidth         : 8 bit or 16 bit input
 *  @param[in] deviceNum        : device on which to run OpenCL kernels
 *  @param[in] useLds           : Should the OpenCL kernel use LDS memory for input
 *
 *  @return bool : true if successful; otherwise false.
 *******************************************************************************
 */
bool init(DeviceInfo *infoDeviceOcl, MedianFilter *paramFF,
                const char *inputImage, cl_int filterSize, 
                cl_uint bitWidth, cl_uint deviceNum, cl_int useLds, cl_uint useIpp)
{
    paramFF->filterSize = filterSize;
    
    /***************************************************************************
     * read the input image                                                   
     ***************************************************************************/
    if (readInput(paramFF, inputImage, bitWidth) == false)
    {
        printf("Error reading input.\n");
        return false;
    }

    /**************************************************************************
    * Initialize the openCL device and create context and command queue      
    ***************************************************************************/
    if (initOpenCl(infoDeviceOcl, deviceNum) == false)
    {
        printf("Error in initOpenCl.\n");
        return false;
    }

    /**************************************************************************
    * Create the memory needed by the pipeline                               
    ***************************************************************************/
    if (createMemory(paramFF, infoDeviceOcl, bitWidth) == false)
    {
        printf("Error in createMemory.\n");
        return false;
    }

    /***************************************************************************
    * Build the Median Filter OpenCL kernel                         
    ***************************************************************************/
    if (buildMedianFilterKernel(infoDeviceOcl->mCtx, infoDeviceOcl->mDevice,
        &(paramFF->medianFilterKernel), filterSize, bitWidth, useLds)
                    == false)
    {
        printf("Error in buildMedianFilterKernel.\n");
        return false;
    }

    /**************************************************************************
    * Sets the Median Filter OpenCL kernel arguments                     
    **************************************************************************/
    if (setMedianFilterKernelArgs(paramFF->medianFilterKernel,
                    paramFF->input, paramFF->output,
                    paramFF->cols, paramFF->rows, paramFF->paddedCols) == false)
    {
        printf("Error in setGaussianFilterKernelArgs.\n");
        return false;
    }
    
    /***************************************************************************
    * Init IPP filter                        
    ***************************************************************************/
    initIppMedianFilter(
        paramFF->filterSize, 
        paramFF->cols, 
        paramFF->rows, 
        bitWidth, 
        &(paramFF->pBuffer));

    return true;
}

/**
 *******************************************************************************
 *  @fn     run
 *  @brief  This function runs the entire pipeline
 *
 *  @param[in/out] infoDeviceOcl : Structure which holds openCL related params
 *  @param[in/out] paramFF      : Structure holds all parameters required
 *                                 by the sample
 *  @param[in] bitWidth         : 8 bit or 16 bit input
 *
 *  @return bool : true if successful; otherwise false.
 *******************************************************************************
 */
bool run(DeviceInfo *infoDeviceOcl, MedianFilter *paramFF, cl_uint bitWidth, cl_uint dataTransfer, cl_event *ev)
{
    cl_int status = 0;

    cl_int paddedRows = paramFF->paddedRows;
    cl_int paddedCols = paramFF->paddedCols;

    if (dataTransfer)
    {
        /**************************************************************************
        * Send the input image data to the device
        ***************************************************************************/
        status = clEnqueueWriteBuffer(infoDeviceOcl->mQueue, paramFF->input,
                        CL_FALSE, 0, paddedRows * paddedCols * sizeof(cl_uchar)
                                        * (bitWidth / 8), paramFF->inputImg, 0,
                        NULL, NULL);
        CHECK_RESULT(status != CL_SUCCESS,
                        "Error in clEnqueueWriteBuffer. Status: %d\n", status);        
    }
    
    /**************************************************************************
     * Run the Median Filter OpenCL kernel.
     ***************************************************************************/
    runMedianFilterKernel(infoDeviceOcl->mQueue, paramFF->medianFilterKernel,
            paramFF->cols, paramFF->rows, ev);
    
    if (dataTransfer) 
    {
        /**************************************************************************
         * Get the results back to host
         ***************************************************************************/
        status = clEnqueueReadBuffer(infoDeviceOcl->mQueue, paramFF->output,
                        CL_TRUE, 0, paramFF->cols * paramFF->rows
                                        * sizeof(cl_uchar) * (bitWidth / 8),
                        paramFF->oclOutputImg, 0, NULL, NULL);
        CHECK_RESULT(status != CL_SUCCESS,
                        "Error in clEnqueueReadBuffer. Status: %d\n", status);
    }

    return true;
}

/**
 *******************************************************************************
 *  @fn     readInput
 *  @brief  This functons reads an input image content and also pads the filter
 *          based on input image size
 *
 *  @param[in] paramFF     : Pointer to structure
 *  @param[in] inputImage : input image file name
 *  @param[in] bitWidth         : 8 bit or 16 bit input
 *
 *  @return bool : true if successful; otherwise false.
 *******************************************************************************
 */
bool readInput(MedianFilter *paramFF, const char *inputImage, cl_uint bitWidth)
{
    uchar4* pixelData;

    // load input bitmap image
    paramFF->inputBitmap.load(inputImage);

    // error if image did not load
    if(!paramFF->inputBitmap.isLoaded())
    {
        printf("Failed to load input image!");
        return false;
    }

    // get width and height of input image
    paramFF->rows = paramFF->inputBitmap.getHeight();
    paramFF->cols = paramFF->inputBitmap.getWidth();

    paramFF->paddedRows = paramFF->rows + paramFF->filterSize - 1;
    paramFF->paddedCols = paramFF->cols + paramFF->filterSize - 1;

    cl_int filterRadius = paramFF->filterSize / 2;

    paramFF->inputImg = (cl_uchar *) malloc(paramFF->paddedCols
                    * paramFF->paddedRows * sizeof(cl_uchar) * bitWidth / 8);
    memset(paramFF->inputImg, 0, paramFF->paddedCols * paramFF->paddedRows
                    * sizeof(cl_uchar) * bitWidth / 8);


    // get the pointer to pixel data
    pixelData = paramFF->inputBitmap.getPixels();
    if(pixelData == NULL)
    {
        printf("Failed to read pixel Data!");
        return false;
    }

    /**************************************************************************
     * Using only r channel of the image. 
     * Pad the input image.
     **************************************************************************/
    if (bitWidth == 16)
    {
        for (cl_uint i = filterRadius; i < paramFF->rows + filterRadius; i++)
        {
            for (cl_uint j = filterRadius; j < paramFF->cols + filterRadius; j++)
            {
                ((cl_ushort *) paramFF->inputImg)[i * paramFF->paddedCols + j]
                                = (cl_ushort)(pixelData[(i - filterRadius) * paramFF->cols + (j - filterRadius)].x);
            }
        }

    }
    else if (bitWidth == 8)
    {
        for (cl_uint i = filterRadius; i < paramFF->rows + filterRadius; i++)
        {
            for (cl_uint j = filterRadius; j < paramFF->cols + filterRadius; j++)
            {
                ((cl_uchar *) paramFF->inputImg)[i * paramFF->paddedCols + j]
                                = (cl_uchar)(pixelData[(i - filterRadius) * paramFF->cols + (j - filterRadius)].x);
            }
        }
    }
    else
    {
        CHECK_RESULT(false, "Un-supported bitWidth, only 8 and 16 bits are supported");
    }

     return true;
}

/**
 *******************************************************************************
 *  @fn     saveOutput
 *  @brief  This functons crops the output image to the size of input image and 
 *          saves it in the given format. Format is identified based on image name
 *
 *  @param[in] paramFF     : Pointer to structure
 *  @param[in] medianOutputImage  : output file name
 *  @param[in] sepOutputImage    : output file name
 *  @param[in] bitWidth         : 8 bit or 16 bit input
 *
 *  @return void
 *******************************************************************************
 */
bool saveOutputs(MedianFilter *paramFF, const char *medianOutputImage,
                const char *ippOutputImage, cl_uint bitWidth)
{
    uchar4 *data = (uchar4 *)malloc(paramFF->cols * paramFF->rows * sizeof(uchar4));
    if (!data) 
    {
        printf("Error mallocing.");
        return false;
    }

    //Save median filter output
    memset(data, 0, paramFF->rows * paramFF->cols * sizeof(uchar4));
    if (bitWidth == 8)
    {      
        for (cl_uint i = 0; i < paramFF->rows; i++)
        {
            for (cl_uint j = 0; j < paramFF->cols; j++)
            {
                data[i * paramFF->cols + j].x = (cl_uchar)paramFF->oclOutputImg[i * paramFF->cols + j];
                data[i * paramFF->cols + j].y = (cl_uchar)paramFF->oclOutputImg[i * paramFF->cols + j];
                data[i * paramFF->cols + j].z = (cl_uchar)paramFF->oclOutputImg[i * paramFF->cols + j];
            }
        }
    }
    else
    {
        for (cl_uint i = 0; i < paramFF->rows; i++)
        {
            for (cl_uint j = 0; j < paramFF->cols; j++)
            {
                data[i * paramFF->cols + j].x = (cl_uchar)((cl_ushort *)paramFF->oclOutputImg)[i * paramFF->cols + j];
                data[i * paramFF->cols + j].y = (cl_uchar)((cl_ushort *)paramFF->oclOutputImg)[i * paramFF->cols + j];
                data[i * paramFF->cols + j].z = (cl_uchar)((cl_ushort *)paramFF->oclOutputImg)[i * paramFF->cols + j];
            }
        }
    }
    // write the output bmp file
    paramFF->inputBitmap.write(medianOutputImage, paramFF->cols, paramFF->rows, (unsigned int *)data);
    
    //Save ipp filter output
    memset(data, 0, paramFF->rows * paramFF->cols * sizeof(uchar4));
    if (bitWidth == 8)
    {      
        for (cl_uint i = 0; i < paramFF->rows; i++)
        {
            for (cl_uint j = 0; j < paramFF->cols; j++)
            {
                data[i * paramFF->cols + j].x = (cl_uchar)paramFF->ippOutputImg[i * paramFF->cols + j];
                data[i * paramFF->cols + j].y = (cl_uchar)paramFF->ippOutputImg[i * paramFF->cols + j];
                data[i * paramFF->cols + j].z = (cl_uchar)paramFF->ippOutputImg[i * paramFF->cols + j];
            }
        }
    }
    else
    {
        for (cl_uint i = 0; i < paramFF->rows; i++)
        {
            for (cl_uint j = 0; j < paramFF->cols; j++)
            {
                data[i * paramFF->cols + j].x = (cl_uchar)((cl_ushort *)paramFF->ippOutputImg)[i * paramFF->cols + j];
                data[i * paramFF->cols + j].y = (cl_uchar)((cl_ushort *)paramFF->ippOutputImg)[i * paramFF->cols + j];
                data[i * paramFF->cols + j].z = (cl_uchar)((cl_ushort *)paramFF->ippOutputImg)[i * paramFF->cols + j];
            }
        }
    }
    // write the output bmp file
    paramFF->inputBitmap.write(ippOutputImage, paramFF->cols, paramFF->rows, (unsigned int *)data);
    
    free(data);

    printf("OpenCL Median Filter output written to %s\n", medianOutputImage);
    printf("ipp Median filter Output written to %s\n\n", ippOutputImage);

    return true;
}

/**
 *******************************************************************************
 *  @fn     createMemory
 *  @brief  This function creates memory required by the pipeline 
 *
 *  @param[in/out] paramFF  : pointer to MedianFilter structure
 *  @param[in] infoDeviceOcl   : pointer to the structure containing opencl 
 *                               device information
 *  @param[in] bitWidth         : 8 bit or 16 bit input
 *
 *  @return bool : true if successful; otherwise false.
 ******************************************************************************
 */
bool createMemory(MedianFilter* paramFF, DeviceInfo *infoDeviceOcl,
                cl_uint bitWidth)
{
    cl_int err = 0;

    int paddedRows = paramFF->paddedRows;
    int paddedCols = paramFF->paddedCols;

    paramFF->input = clCreateBuffer(infoDeviceOcl->mCtx, CL_MEM_READ_ONLY,
                        paddedRows * paddedCols * sizeof(cl_uchar) * (bitWidth / 8), 
                        NULL, &err);
    CHECK_RESULT(err != CL_SUCCESS, "clCreateBuffer failed with %d\n", err);

    paramFF->output = clCreateBuffer(infoDeviceOcl->mCtx, CL_MEM_WRITE_ONLY,
                    paramFF->rows * paramFF->cols * sizeof(cl_uchar)
                                    * (bitWidth / 8), NULL, &err);
    CHECK_RESULT(err != CL_SUCCESS, "clCreateBuffer failed with %d\n", err);

    paramFF->oclOutputImg = (cl_uchar *) malloc(paramFF->rows * paramFF->cols
                    * sizeof(cl_uchar) * (bitWidth / 8));
    CHECK_RESULT(paramFF->oclOutputImg == NULL, "Malloc failed.\n");

    paramFF->ippOutputImg = (cl_uchar *) malloc(paramFF->rows * paramFF->cols
                    * sizeof(cl_uchar) * (bitWidth / 8));
        CHECK_RESULT(paramFF->ippOutputImg == NULL, "Malloc failed.\n");

    return true;
}

/**
 *******************************************************************************
 *  @fn     destroyMemory
 *  @brief  This function destroys the memory created by the pipeline
 *
 *  @param[in/out] paramFF  : pointer to structure
 *  @param[in] infoDeviceOcl   : pointer to the structure containing opencl
 *                               device information
 *
 *  @return void
 *******************************************************************************
 */
void destroyMemory(MedianFilter* paramFF, DeviceInfo *infoDeviceOcl)
{
    free(paramFF->inputImg);
    free(paramFF->oclOutputImg);
    free(paramFF->ippOutputImg);

    ippFree(paramFF->pBuffer);

    clReleaseMemObject(paramFF->input);
    clReleaseMemObject(paramFF->output);
    clReleaseKernel(paramFF->medianFilterKernel);
}