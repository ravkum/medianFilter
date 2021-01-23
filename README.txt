Pre-requisites:
1) On BaldEagle machine: 
	1) Install Visual Studio 2012.
	2) Install AMD Catalyst driver.
	3) install AMD APP SDK 2.9.1. 
	4) Install IPP (Intel parallel_studio_xe_2015 was used to test, but any package with IPP should be good). 
	5) Set IPPROOT environment variable and add "%IPPROOT%\..\redist\intel64\ipp" to PATH environment variable.


2) On Intel machine:
	1) Install Visual Studio 2012.
	2) Install Intel graphics driver.
	3) Install Intel OpenCL SDK.
	4) Install IPP (Intel parallel_studio_xe_2015 Intel parallel_studio_xe_2015 was used to test, but any package with IPP should be good). 
	5) Set IPPROOT environment variable and add "%IPPROOT%\..\redist\intel64\ipp" to PATH environment variable.
	

Steps to compile the project:
1) Open the .sln file in Visual Studio 2012.
2) Change the configuration to Relase | x64.
3) Only on Intel machine, right-click on project in the Visual Studio solution exporer -> "Intel Compiler -> Use Intel C++". Do not do this for AMD platform. 
4) Build the project.


Steps to run the exe:
1) Goto "medianFilter -> bin -> Release -> x86_64"
2) Run medianFilter.exe.


Exe command line options:
1) -i: input image path
2) -bitWidth : Bits per channel to be used (8 | 16)
2) -filtSize : filterSize supported currently is 3 or 5
3) -useLds : Should OpenCL kernel use LDS memory to store input data (0 | 1)
4) -verify : Verifies the OpenCL output against the IPP output (0 | 1)


Example: 
1) To run 5X5 filters on 16 bit/channel input image, run:
 
	medianFilter.exe -i Nature_2448x2044.bmp -filtSize 5 -bitWidth 16 -useLds 0 -verify 1

2) To run 3X3 filters on 16 bit/channel input image, run:
 
	medianFilter.exe -i Nature_2448x2044.bmp -filtSize 3 -bitWidth 16 -useLds 0 -verify 1
