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
 * @file <ippMedianFilter.cpp>
 *
 * @brief This file implements functions related to IPP Median filter.
 *
 ********************************************************************************
 */

#include"ippMedianFilter.h"
#include <stdio.h>

bool initIppMedianFilter(cl_uint filterSize, cl_uint width, cl_uint height, cl_uint bitWidth,
                    Ipp8u** pBuffer)    
{
    ippInit();

    IppStatus status;
    IppiSize dstRoiSize = {width, height};
    int bufferSize;

    IppDataType inputDataType = (bitWidth == 8) ? ipp8u : ipp16u;
    IppiSize  maskSize = {filterSize, filterSize};

    status = ippiFilterMedianBorderGetBufferSize(dstRoiSize, maskSize, inputDataType, 1, &bufferSize);
    if (status != ippStsNoErr)
    {
        printf("Error in ippiFilterMedianBorderGetBufferSize: %s\n", ippGetStatusString(status));
        return false;
    }
    
    *pBuffer = (Ipp8u*)ippsMalloc_8u(bufferSize);

    return true;
}

bool runIppMedianFilter(cl_uchar *inputImg, cl_uint filterSize, cl_uchar *outputImg, cl_uint width, cl_uint height, cl_uint bitWidth,
                  Ipp8u* pBuffer)
{
    cl_uint paddedWidth = width + filterSize - 1;

    IppStatus status;
    IppiSize dstRoiSize = {width, height};
    Ipp8u borderValue_8u = 0;
    Ipp16u borderValue_16u = 0;
    IppiSize  maskSize = {filterSize, filterSize};
    IppiBorderType borderType = (IppiBorderType)(ippBorderConst | ippBorderInMemTop | ippBorderInMemRight);
    
    if (bitWidth == 8)
    {
        status = ippiFilterMedianBorder_8u_C1R(
            (inputImg + ((filterSize / 2) * paddedWidth + (filterSize / 2)) * (bitWidth / 8)), 
            paddedWidth * (bitWidth / 8), 
            outputImg, 
            width * (bitWidth / 8), 
            dstRoiSize, 
            maskSize, 
            borderType, 
            0, 
            pBuffer);        
    }
    else
    {
        status = ippiFilterMedianBorder_16u_C1R(
            (const Ipp16u *)(inputImg + ((filterSize / 2) * paddedWidth + (filterSize / 2)) * (bitWidth / 8)), 
            paddedWidth * (bitWidth / 8), 
            (Ipp16u *)outputImg, 
            width * (bitWidth / 8), 
            dstRoiSize, 
            maskSize, 
            borderType, 
            0, 
            pBuffer);              
    }

    if (status != ippStsNoErr)
    {
        printf("Error in ippiFilterMedianBorder_%du_C1R: %s\n", bitWidth, ippGetStatusString(status));
        return false;
    }

    return true;
}