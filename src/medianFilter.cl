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

#if PIX_WIDTH == 8
#define T1 uchar
#define ROUND(x) ((x > 0) ? convert_uchar_rtz(x) : 0)
#else
#define T1 ushort
#define ROUND(x) ((x > 0) ? convert_ushort_rtz(x) : 0)
#endif

#define OP(a,b) {  T1 mid=a; a=min(a,b); b=max(mid,b);}

__attribute__((always_inline)) T1 get_median_3 (T1 *p)
{
    OP(p[1], p[2]); OP(p[4], p[5]); OP(p[7], p[8]); OP(p[0], p[1]); 
    OP(p[3], p[4]); OP(p[6], p[7]); OP(p[1], p[2]); OP(p[4], p[5]); 
    OP(p[7], p[8]); OP(p[0], p[3]); OP(p[5], p[8]); OP(p[4], p[7]); 
    OP(p[3], p[6]); OP(p[1], p[4]); OP(p[2], p[5]); OP(p[4], p[7]); 
    OP(p[4], p[2]); OP(p[6], p[4]); OP(p[4], p[2]); 

    return p[4];
}

__attribute__((always_inline))  T1 get_median_5(T1 *p)
{
    OP(p[0], p[1]) ; OP(p[3], p[4]) ; OP(p[2], p[4]) ;
    OP(p[2], p[3]) ; OP(p[6], p[7]) ; OP(p[5], p[7]) ;
    OP(p[5], p[6]) ; OP(p[9], p[10]) ; OP(p[8], p[10]) ;
    OP(p[8], p[9]) ; OP(p[12], p[13]) ; OP(p[11], p[13]) ;
    OP(p[11], p[12]) ; OP(p[15], p[16]) ; OP(p[14], p[16]) ;
    OP(p[14], p[15]) ; OP(p[18], p[19]) ; OP(p[17], p[19]) ;
    OP(p[17], p[18]) ; OP(p[21], p[22]) ; OP(p[20], p[22]) ;
    OP(p[20], p[21]) ; OP(p[23], p[24]) ; OP(p[2], p[5]) ;
    OP(p[3], p[6]) ; OP(p[0], p[6]) ; OP(p[0], p[3]) ;
    OP(p[4], p[7]) ; OP(p[1], p[7]) ; OP(p[1], p[4]) ;
    OP(p[11], p[14]) ; OP(p[8], p[14]) ; OP(p[8], p[11]) ;
    OP(p[12], p[15]) ; OP(p[9], p[15]) ; OP(p[9], p[12]) ;
    OP(p[13], p[16]) ; OP(p[10], p[16]) ; OP(p[10], p[13]) ;
    OP(p[20], p[23]) ; OP(p[17], p[23]) ; OP(p[17], p[20]) ;
    OP(p[21], p[24]) ; OP(p[18], p[24]) ; OP(p[18], p[21]) ;
    OP(p[19], p[22]) ; OP(p[8], p[17]) ; OP(p[9], p[18]) ;
    OP(p[0], p[18]) ; OP(p[0], p[9]) ; OP(p[10], p[19]) ;
    OP(p[1], p[19]) ; OP(p[1], p[10]) ; OP(p[11], p[20]) ;
    OP(p[2], p[20]) ; OP(p[2], p[11]) ; OP(p[12], p[21]) ;
    OP(p[3], p[21]) ; OP(p[3], p[12]) ; OP(p[13], p[22]) ;
    OP(p[4], p[22]) ; OP(p[4], p[13]) ; OP(p[14], p[23]) ;
    OP(p[5], p[23]) ; OP(p[5], p[14]) ; OP(p[15], p[24]) ;
    OP(p[6], p[24]) ; OP(p[6], p[15]) ; OP(p[7], p[16]) ;
    OP(p[7], p[19]) ; OP(p[13], p[21]) ; OP(p[15], p[23]) ;
    OP(p[7], p[13]) ; OP(p[7], p[15]) ; OP(p[1], p[9]) ;
    OP(p[3], p[11]) ; OP(p[5], p[17]) ; OP(p[11], p[17]) ;
    OP(p[9], p[17]) ; OP(p[4], p[10]) ; OP(p[6], p[12]) ;
    OP(p[7], p[14]) ; OP(p[4], p[6]) ; OP(p[4], p[7]) ;
    OP(p[12], p[14]) ; OP(p[10], p[14]) ; OP(p[6], p[7]) ;
    OP(p[10], p[12]) ; OP(p[6], p[10]) ; OP(p[6], p[17]) ;
    OP(p[12], p[17]) ; OP(p[7], p[17]) ; OP(p[7], p[10]) ;
    OP(p[12], p[18]) ; OP(p[7], p[12]) ; OP(p[10], p[18]) ;
    OP(p[12], p[20]) ; OP(p[10], p[20]) ; OP(p[10], p[12]) ;

    return (p[12]);
}

__kernel 
__attribute__((reqd_work_group_size(LOCAL_XRES, LOCAL_YRES, 1)))
void medianFilter(
                    __global T1 *input,
                    __global T1 *output,
                    uint nWidth,
                    uint nHeight,
                    uint nExWidth
                    )
{    
    int filterRadius = FILTERSIZE / 2;

    int col = get_global_id(0);
    int row = get_global_id(1);

    if (col >= nWidth || row >= nHeight) return;

    int xsize = nExWidth;
	
    int start_col, start_row;
    
    T1 private_input[FILTERSIZE * FILTERSIZE];
	
    /***************************************************************************************
    * If using LDS, get the data to local memory. Else, get the global memory indices ready 
    ***************************************************************************************/
#if USE_LDS == 1
    __local T1 local_input[(LOCAL_XRES + FILTERSIZE - 1) * (LOCAL_YRES + FILTERSIZE - 1)];
    
    int tile_xres = (LOCAL_XRES + FILTERSIZE - 1);
    int tile_yres = (LOCAL_YRES + FILTERSIZE - 1);

    int lid_x = get_local_id(0);
    int lid_y = get_local_id(1);

    start_col = get_group_id(0) * LOCAL_XRES; //Image is padded
    start_row = get_group_id(1) * LOCAL_YRES; 

    int lid = lid_y * LOCAL_XRES + lid_x; 
    int gx, gy;

    /*********************************************************************
    * Read input from global buffer and put in local buffer 
    * Read 256 global memory locations at a time (256 WI). 
    * Conitnue in a loop till all pixels in the tile are read.
    **********************************************************************/
    do {
        gy = lid / tile_xres;
        gx = lid - gy * tile_xres;         
        local_input[lid] = input[(start_row + gy) * xsize + (start_col + gx)];
        lid += (LOCAL_XRES * LOCAL_YRES);
    } while (lid < (tile_xres * tile_yres));

    barrier(CLK_LOCAL_MEM_FENCE);

#else 
    /************************************************************************
    * Non - LDS implementation
    * Read pixels directly from global memory
    ************************************************************************/
	start_col = col; 
	start_row = row; 	
#endif 
	
    /***************************************************************************************
    * Read memory into the private array
    ***************************************************************************************/
#pragma unroll FILTERSIZE	
    for (int i = 0; i < FILTERSIZE; i++) {
#pragma unroll FILTERSIZE
        for (int j = 0; j < FILTERSIZE; j++) {
#if USE_LDS == 1    
            private_input[i * FILTERSIZE + j] = local_input[(lid_y + i) * tile_xres + (lid_x + j)]; 
#else               
            private_input[i * FILTERSIZE + j] = input[(start_row + i) * xsize + (start_col + j)]; 
#endif
        }
    }

    /***************************************************************************************
    * Get the median value of the array
    ***************************************************************************************/
#if FILTERSIZE == 3
    T1 out_val = get_median_3(private_input);
#else
    T1 out_val = get_median_5(private_input);
#endif

    /***************************************************************************************
    * Save Output
    ***************************************************************************************/
    output[row * nWidth + col] = out_val;
}