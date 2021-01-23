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
 * @file <medianFilter.cpp>
 *
 * @brief Contains functions to compile and run the median filter kernel
 *
 ********************************************************************************
 */
#include "medianFilter.h"
/**
 *******************************************************************************
 *  @fn     buildKernelMedianFilter
 *  @brief  This function builds the OpenCL median filter kernel
 *
 *  @param[in] oclCtx        : pointer to the Ocl context
 *  @param[in] oclDevice     : pointer to the ocl device
 *  @param[out] medianFilter : pointer to the kernel
 *  @param[in] filtSize      : Filter size if set to = 3 - 3x3 Median filter
 *                                                   = 5 - 5x5 Median filter
 *  @param[in] bitWidth      : Bytes per pixel
 *  @param[in] useLds        : Should the OpenCL kernel use LDS memory for input
 *
 *  @return bool : true if successful; otherwise false.
 *******************************************************************************
 */
bool buildMedianFilterKernel(cl_context oclCtx, cl_device_id oclDevice,
                cl_kernel *medianFilter, cl_uint filtSize, cl_uint bitWidth,
                cl_int useLds)
{
    const char *filename = MEDIANFILTER_KERNEL_SOURCE;

    filename = MEDIANFILTER_KERNEL_SOURCE;

    cl_int err;
    cl_program programMedianFitler;
    char *source;
    size_t sourceSize;
    err = convertToString(filename, &source, &sourceSize);
    CHECK_RESULT(err != CL_SUCCESS, "Error reading file %s ", filename);

    programMedianFitler
                    = clCreateProgramWithSource(oclCtx, 1,
                                    (const char **) &source,
                                    (const size_t *) &sourceSize, &err);
    CHECK_RESULT(err != CL_SUCCESS,
                    "clCreateProgramWithSource failed with Error code = %d",
                    err);

    /**************************************************************************
     * Build the kernel and check for errors. If errors are found, it will be  *
     * dumped into buildlog.txt                                                *
     **************************************************************************/
    char option[256];
    sprintf(option, "-DPIX_WIDTH=%d -DFILTERSIZE=%d -DLOCAL_XRES=%d -DLOCAL_YRES=%d -DUSE_LDS=%d",
                    bitWidth, filtSize, LOCAL_XRES, LOCAL_YRES, useLds);
    err = clBuildProgram(programMedianFitler, 1, &(oclDevice), option, NULL,
                    NULL);
    free(source);
    if (err != CL_SUCCESS)
    {
        char *buildLog = NULL;
        size_t buildLogSize = 0;

        clGetProgramBuildInfo(programMedianFitler, oclDevice,
                        CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog,
                        &buildLogSize);
        buildLog = (char *) malloc(buildLogSize);
        clGetProgramBuildInfo(programMedianFitler, oclDevice,
                        CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, NULL);

        printf("%s\n", buildLog);
        free(buildLog);

        clReleaseProgram(programMedianFitler);
        CHECK_RESULT(false, "clCreateProgram failed with Error code = %d", err);

    }

    *medianFilter = clCreateKernel(programMedianFitler, MEDIANFILTER_KERNEL,
                    &err);
    clReleaseProgram(programMedianFitler);
    return true;
}

/**
 *******************************************************************************
 *  @fn     setMedianFilterKernelArgs
 *  @brief  This function sets the arguments for median filter kernel
 *
 *  @param[in] medianFilter : pointer to median filter kernel
 *  @param[in] input        : Ocl memory containing input 
 *  @param[out] output      : Ocl memory to hold output 
 *  @param[in] width        : Image width
 *  @param[in] height       : Image height
 *  @param[in] paddedWidth  : padded width of the image
 *
 *  @return bool : true if successful; otherwise false.
 *******************************************************************************
 */
bool setMedianFilterKernelArgs(cl_kernel medianFilter, cl_mem input,
                cl_mem output, cl_uint width, cl_uint height,
                cl_uint paddedWidth)
{
    int cnt = 0;
    cl_int err = 0;

    err = clSetKernelArg(medianFilter, cnt++, sizeof(cl_mem), &(input));
    err |= clSetKernelArg(medianFilter, cnt++, sizeof(cl_mem), &(output));
    err |= clSetKernelArg(medianFilter, cnt++, sizeof(cl_int), &(width));
    err |= clSetKernelArg(medianFilter, cnt++, sizeof(cl_int), &(height));
    err |= clSetKernelArg(medianFilter, cnt++, sizeof(cl_int), &(paddedWidth));

    CHECK_RESULT(err != CL_SUCCESS,
                    "clSetKernelArg failed with Error code = %d", err);
    return true;

}

/**
 *******************************************************************************
 *  @fn     runMedianFilterKernel
 *  @brief  This function runs the median filter kenrel 
 *
 *  @param[in] oclQueue        : pointer to the ocl command queue
 *  @param[in] medianFilter    : pointer to the row processing kernel
 *  @param[in] width           : X dimention
 *  @param[in] height          : Y dimention
 *
 *  @return bool : true if successful; otherwise false.
 *******************************************************************************
 */
bool runMedianFilterKernel(cl_command_queue oclQueue, cl_kernel medianFilter,
                cl_uint width, cl_uint height, cl_event *ev)
{
    cl_int err;
    size_t localWorkSize[2] = { LOCAL_XRES, LOCAL_YRES };
    size_t globalWorkSize[2];

    globalWorkSize[0] = (width + localWorkSize[0] - 1) / localWorkSize[0];
    globalWorkSize[0] *= localWorkSize[0];
    globalWorkSize[1] = (height + localWorkSize[1] - 1) / localWorkSize[1];
    globalWorkSize[1] *= localWorkSize[1];

    err = clEnqueueNDRangeKernel(oclQueue, medianFilter, 2, NULL,
                    globalWorkSize, localWorkSize, 0, NULL, ev);
    CHECK_RESULT(err != CL_SUCCESS,
                    "clEnqueueNDRangeKernel failed with Error code = %d", err);

    return true;
}
