# WaveRange

I. GENERAL INFORMATION

WaveRange is a utility for compression of three-dimensional array output from computational physics solvers. It uses wavelet decomposition and subsequent range coding with quantization suitable for floating-point data.  

References:

[1] 
D. Kolomenskiy, R. Onishi, H. Uehara "WaveRange: wavelet-based data compression for three-dimensional numerical simulations on regular grids" 
Journal of Visualization, accepted manuscript, 2022. https://doi.org/10.1007/s12650-021-00813-8
(preprint: http://arxiv.org/abs/1810.04822)

[2]
doc/cfdproc2017.pdf
Dmitry Kolomenskiy, Ryo Onishi and Hitoshi Uehara "Wavelet-Based Compression of CFD Big Data"
Proceedings of the 31st Computational Fluid Dynamics Symposium, Kyoto, December 12-14, 2017
Paper No. C08-1

This work is supported by the FLAGSHIP2020, MEXT within the priority study4 
(Advancement of meteorological and global environmental predictions utilizing 
observational “Big Data”).

Copyright (C) 2017  Dmitry Kolomenskiy

Copyright (C) 2017  Ryo Onishi

Copyright (C) 2017  JAMSTEC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

II. COMPILING AND BUILDING

1) Modify the 'config.mk' file. Set the environment variables 'CC' and 'CXX' to point to the desired compilers, set the compiler flags in 'CFLAGS' and 'CXXFLAGS', and the archiver in 'AR'. 
* HDF5 (<https://www.hdfgroup.org/downloads/hdf5/>) and MPI are required for building the FluSI interface. If compiling with HDF5 support, modify the paths in 'HDF_INC' and 'HDF_LIB' to point to the valid library files. It may be necessary to use mpicxx or h5c++. 
* If not using HDF5, select a serial compiler and empty 'HDF_INC' and 'HDF_LIB'.
2) Type 'make' to build the executable files. To only build one of the interfaces, type 'make generic', 'make flusi' or 'make mssg'.
3) Executables will appear in 'bin/' directory. Its sub-directory 'bin/generic/' will contain the utilities for compressing plain unformatted Fortran or C/C++ floating-point output files. 'bin/flusi/' will contain compression and reconstruction utilities for FluSI output data, 'bin/mssg/' will contain similar utilities for MSSG data. The encoder executable file names end with 'enc', the decoder executable file names end with 'dec'. Library files will appear in 'bin/lib/' and 'bin/include/'.

III. USING WAVERANGE AS A STANDALONE APPLICATION

1) Copy the executables into the same directory with the data files. Sample compressed data files can be downloaded from <https://osf.io/pz4n8/>. 
2) There are three different ways to let WaveRange know how to compress/reconstruct the data.
* Interactively using a command line. Run the program ('wrenc', 'wrdec', 'wrmssgenc' or 'wrmssgdec') and follow the command prompt.
* Using command files. The contents of these command files substitute for the standard input. Sample command files 'inmeta' and 'outmeta' can be found in 'examples/generic/', 'examples/flusi/' and 'examples/mssg/'.
* Using a parameter string. 

   The 'generic/' interface accepts the following input parameters in the compression mode: './wrenc INPUT_FILE ENCODED_FILE HEADER_FILE TYPE ENDIANFLIP NF PRECISION TOLERANCE NX NY NZ'; where INPUT_FILE is the input floating point data file name, ENCODED_FILE is the encoded output data and HEADER_FILE is the output header file names, TYPE=(0: Fortran sequential with 4-byte record length; 1: Fortran sequential with 8-byte record length; 2: C/C++), ENDIANFLIP=(convert little endian to big endian and vice-versa, 0:no; 1:yes), NF=(how many fields, e.g. 1), PRECISION=(floating point precision, 1:single; 2:double), NX=(first dimension, e.g. 16), NY=(second dimension, e.g. 16), NZ=(third dimension, e.g. 16) and TOLERANCE=(relative tolerance, e.g. 1.0e-16). It accepts the following parameters in the reconstruction mode: './wrdec ENCODED_FILE HEADER_FILE EXTRACTED_FILE TYPE ENDIANFLIP'; where ENCODED_FILE and HEADER_FILE are the input compressed data and header file names, EXTRACTED_FILE is the extracted output file name, TYPE=(0: Fortran sequential with 4-byte record length; 1: Fortran sequential with 8-byte record length; 2: C/C++) and ENDIANFLIP=(convert little endian to big endian and vice-versa, 0:no; 1:yes). 

   The 'flusi/' interface takes the input parameters as follows: './wrenc original_000.h5 compressed_000.h5 TYPE TOLERANCE'; './wrdec compressed_000.h5 decompressed_000.h5 TYPE PRECISION'; where TYPE=(0: regular output; 1: backup), PRECISION=(floating point precision, 1:single; 2:double) and TOLERANCE=(relative tolerance such as 1.0e-5 etc).

   The 'mssg/' interface takes the following input parameters in the compression mode: './wrmssgenc FILE_NAME_PREFIX ENCODED_NAME_EXT TYPE PRECISION ENDIANFLIP TOLERANCE PROCID'; where TYPE=(0: regular output; 1: backup united; 2: backup divided), PRECISION=(floating point precision, 1:single; 2:double), ENDIANFLIP=(convert little endian to big endian and vice-versa, 0:no; 1:yes), TOLERANCE=(e.g. 1.0e-16) and PROCID=(this subdomain proc id used in the divided output, otherwise zero). It takes the following parameters in the reconstruction mode: './wrmssgdec ENCODED_NAME_PREFIX ENCODED_NAME_EXT EXTRACTED_NAME_PREFIX TYPE PRECISION ENDIANFLIP PROCID'; where TYPE=(0: regular output; 1: backup united; 2: backup divided), PRECISION=(floating point precision, 1:single; 2:double), ENDIANFLIP=(convert little endian to big endian and vice-versa, 0:no; 1:yes) and PROCID=(this subdomain proc id used in the divided output, otherwise zero).

3) Examples.

* Create a sample Fortran unformatted sequential access binary file 'data.bin' that contains a 32x32x32 double precision array, a 64x64x64 double precision array and 1 single precision variable. Modify 'Makefile' if necessary and type 'make' to build an executable 'create_in_field'. Compress the first array with 1e-6 relative tolerance, the second array with 1e-3 relative tolerance, and leave the last single precision variable uncompressed. Finally, reconstruct the data from the compressed format. This example assumes that the record length is 4 bit. No endian conversion is performed. 

    $ cd examples/generic

    $ ./create_in_field

    $ ./wrenc < inmeta

    $ ./wrdec < outmeta

  The contents of the command file 'inmeta' substitute for the following interactive input:

   Enter input data file name [data.bin]: data.bin

   Enter encoded data file name [data.wrb]: data.wrb

   Enter encoding header file name [data.wrh]: data.wrh

   Enter file type (0: Fortran sequential w 4-byte recl; 1: Fortran sequential w 8-byte recl; 2: C/C++) [0]: 0

   Enter endian conversion (0: do not perform; 1: inversion) [0]: 0

   Enter the number of fields in the file, nf [1]: 3

   Field number 0

   Enter input data type (1: float; 2: double) [2]: 2

   Enter the number of data points in the first dimension, nx [16]: 32

   Enter the number of data points in the second dimension, ny [16]: 32

   Enter the number of data points in the third dimension, nz [16]: 32

   Enter the number of data points in the higher (slowest) dimensions, nh [1]: 1

   Invert the order of the dimensions? (0: no; 1: yes) [0]: 0

   Enter compression flag (0: do not compress; 1: compress) [1]: 1

   Enter base cutoff relative tolerance [1e-16]: 1e-6

   Field number 1

   Enter input data type (1: float; 2: double) [2]: 2

   Enter the number of data points in the first dimension, nx [16]: 64

   Enter the number of data points in the second dimension, ny [16]: 64

   Enter the number of data points in the third dimension, nz [16]: 64

   Enter the number of data points in the higher (slowest) dimensions, nh [1]: 1

   Invert the order of the dimensions? (0: no; 1: yes) [0]: 0

   Enter compression flag (0: do not compress; 1: compress) [1]: 1

   Enter base cutoff relative tolerance [1e-16]: 1e-3

   Field number 2

   Enter input data type (1: float; 2: double) [2]: 1

   Enter the number of data points in the first dimension, nx [16]: 1

   Enter the number of data points in the second dimension, ny [16]: 1

   Enter the number of data points in the third dimension, nz [16]: 1

   Enter the number of data points in the higher (slowest) dimensions, nh [1]: 1

   Invert the order of the dimensions? (0: no; 1: yes) [0]: 0

   Enter compression flag (0: do not compress; 1: compress) [1]: 0

  The contents of the command file 'outmeta' substitute for the following interactive input:

   Enter encoded data file name [data.wrb]: data.wrb

   Enter encoding header file name [data.wrh]: data.wrh

   Enter extracted (output) data file name [datarec.bin]: datarec.bin

   Enter file type (0: Fortran sequential w 4-byte recl; 1: Fortran sequential w 8-byte recl; 2: C/C++) [0]: 0

   Enter endian conversion (0: do not perform; 1: inversion) [0]: 0

* Download compressed FluSI regular output data in HDF5 format, reconstruct and compress with 1e-3 tolerance using the command line.

     $ cd examples/flusi

     $ wget https://osf.io/5kcuq/download --output-document=ux_hit.enc.h5

     $ ./wrdec ux_hit.enc.h5 ux_hit.h5 0 2

     $ ./wrenc ux_hit.h5 ux_hit_lr.enc.h5 0 1e-3

* Download compressed FluSI regular output data in HDF5 format, reconstruct and compress with 1e-3 tolerance using command files.

     $ wget https://osf.io/5kcuq/download --output-document=ux_hit.enc.h5

     $ ./wrdec < outmeta

     $ ./wrenc < inmeta

  The contents of the sample command file 'outmeta' substitute for the following interactive input:

   Enter compressed data file name []: ux_hit.enc.h5

   Enter reconstructed file name []: ux_hit.h5

   Enter file type (0: regular output; 1: backup) [0]: 0

   Enter output data type (1: float; 2: double) [2]: 2

  The contents of the sample command file 'inmeta' substitute for the following interactive input:

   Enter input file name []: ux_hit.h5

   Enter output file name []: ux_hit_lr.enc.h5

   Enter file type (0: regular output; 1: backup) [0]: 0

   Enter base cutoff relative tolerance [1e-16]: 1e-3

IV. USING WAVERANGE AS A LIBRARY

All compilers will produce a static library file 'bin/lib/libwaverange.a'. A C++ header file 'wrappers.h' containing the encoding and decoding function definitions will be copied to 'bin/include/'. In addition, if the C compiler name in 'config.mk' is defined as 'CC = gcc', a shared library 'bin/lib/libwaverange.so' will be generated. To build your application with WaveRange, add '-L$(WAVERANGE_LIBRARY_PATH) -lwaverange -lstdc++' at linkage, see an example in 'examples/fortran/Makefile'. 

NOTE: The compression routines 'encoding_wrap' and 'encoding_wrap_f' overwrite the input floating-point array with temporary data.

1) C++ interface.

* extern "C" void encoding_wrap(int nx, int ny, int nz, double *fld_1d, int wtflag, int mx, int my, int mz, double *cutoffvec, double& tolabs, double& midval, double& halfspanval, unsigned char& wlev, unsigned char& nlay, unsigned long int& ntot_enc, double *deps_vec, double *minval_vec, unsigned long int *len_enc_vec, unsigned char *data_enc); // Compression

   nx : (INPUT) number of elements of the input 3D field in the first (fastest) direction

   ny : (INPUT) number of elements of the input 3D field in the second direction

   nz : (INPUT) number of elements of the input 3D field in the third (slowest) direction

   fld_1d : (INPUT) input 3D field reshaped in a 1D array with the direction x being contiguous (as in Fortran)

   wtflag : (INPUT) wavelet transform flag: 0 if not transforming, 1 if transforming

   mx : (INPUT) number local cutoff subdomains in x, recommended mx=1

   my : (INPUT) number local cutoff subdomains in y, recommended my=1

   mz : (INPUT) number local cutoff subdomains in z, recommended mz=1

   cutoffvec : (INPUT) local cutoff vector, e.g. cutoffvec = new double[1]; cutoffvec[0] = relative_global_tolerance;

   tolabs : (OUTPUT) absolute global tolerance

   midval : (OUTPUT) mid-value, midval = minval+(maxval-minval)/2, where maxval is the maximum and minval is the minimum of fld_1d

   halfspanval : (OUTPUT) half-span, halfspanval = (maxval-minval)/2

   wlev : (OUTPUT) number of wavelet transform levels

   nlay : (OUTPUT) number of bit planes

   ntot_enc : (OUTPUT) total number of elements of the encoded array data_enc

   deps_vec : (OUTPUT) quantization step size vector, double deps_vec[nlaymax]; where nlaymax is an output of setup_wr

   minval_vec : (OUTPUT) bit plane offset vector, double minval_vec[nlaymax];

   len_enc_vec : (OUTPUT) number of elements in the encoded bit planes, unsigned long int len_enc_vec[nlaymax];

   data_enc : (OUTPUT) range-encoded output data array, defined as, e.g., unsigned char *data_enc = new unsigned char[ntot_enc_max]; where ntot_enc_max is an output of setup_wr */ 

* extern "C" void decoding_wrap(int nx, int ny, int nz, double *fld_1d, double& tolabs, double& midval, double& halfspanval, unsigned char& wlev, unsigned char& nlay, unsigned long int& ntot_enc, double *deps_vec, double *minval_vec, unsigned long int *len_enc_vec, unsigned char *data_enc); // Reconstruction

   nx : (INPUT) number of elements of the input 3D field in the first (fastest) direction

   ny : (INPUT) number of elements of the input 3D field in the second direction

   nz : (INPUT) number of elements of the input 3D field in the third (slowest) direction

   fld_1d : (OUTPUT) output reconstructed 3D field reshaped in a 1D array with the direction x being contiguous (as in Fortran)

   tolabs : (NOT USED)

   midval : (INPUT) mid-value, midval = minval+(maxval-minval)/2, where maxval is the maximum and minval is the minimum of fld_1d

   halfspanval : (INPUT) half-span, halfspanval = (maxval-minval)/2

   wlev : (INPUT) number of wavelet transform levels

   nlay : (INPUT) number of bit planes

   ntot_enc : (INPUT) total number of elements of the encoded array data_enc

   deps_vec : (INPUT) quantization step size vector, double deps_vec[nlay];

   minval_vec : (INPUT) bit plane offset vector, double minval_vec[nlay];

   len_enc_vec : (INPUT) number of elements in the encoded bit planes, unsigned long int len_enc_vec[nlay];

   data_enc : (INPUT) range-encoded data array, defined as, e.g., unsigned char *data_enc = new unsigned char[ntot_enc];

* extern "C" void setup_wr(int nx, int ny, int nz, unsigned char& nlaymax, unsigned long int& ntot_enc_max); // Return number of bit planes and max output data size - call before encoding_wrap

   nx : (INPUT) number of elements of the input 3D field in the first (fastest) direction

   ny : (INPUT) number of elements of the input 3D field in the second direction

   nz : (INPUT) number of elements of the input 3D field in the third (slowest) direction

   nlaymax : (OUTPUT) maximum allowed number of bit planes

   ntot_enc_max : (OUTPUT) maximum allowed total number of elements of the encoded array data_enc

2) Fortran interface. For the functional description of all input/output parameters, see the C++ interface comments above. For a working example, see examples/fortran/.

* subroutine encoding_wrap_f(nx, ny, nz, fld, wtflag, tolrel, tolabs, midval, halfspanval, wlev, nlay, ntot_enc, deps_vec, minval_vec, len_enc_vec, data_enc) ! Compression

   byte :: wlev, nlay ! OUTPUT

   byte, allocatable :: data_enc(:) ! OUTPUT

   integer :: nx,ny,nz,wtflag ! INPUT

   integer*8 :: ntot_enc ! OUTPUT

   integer*8, allocatable :: len_enc_vec(:) ! OUTPUT

   double precision :: tolrel ! INPUT - relative global tolerance

   double precision :: midval,halfspanval,tolabs ! OUTPUT

   double precision, allocatable :: deps_vec(:),minval_vec(:) ! OUTPUT

   double precision, allocatable :: fld(:,:,:) ! INPUT

* subroutine decoding_wrap_f(nx, ny, nz, fld, midval, halfspanval, wlev, nlay, ntot_enc, deps_vec, minval_vec, len_enc_vec, data_enc) ! Reconstruction

   byte :: wlev, nlay ! INPUT

   byte, allocatable :: data_enc(:) ! INPUT

   integer :: nx,ny,nz ! INPUT 

   integer*8 :: ntot_enc ! INPUT

   integer*8, allocatable :: len_enc_vec(:) ! INPUT

   double precision :: midval,halfspanval ! INPUT

   double precision, allocatable :: deps_vec(:),minval_vec(:) ! INPUT

   double precision, allocatable :: fld(:,:,:) ! OUTPUT

* subroutine setup_wr_f(nx, ny, nz, nlaymax, ntot_enc_max) ! Return number of bit planes and max output data size - call before encoding_wrap_f

   integer :: nx,ny,nz ! INPUT 

   integer :: nlaymax ! OUTPUT

   integer*8 :: ntot_enc_max ! OUTPUT

V. BRIEF DESCRIPTION OF THE FILES

* config.mk : build configuration file
* LICENSE.txt : license file
* Makefile : make file
* README.md : this readme file
* doc/cfdproc2017.pdf : brief theoretical introduction
* src/core/Makefile : encoding/decoding functions make file
* src/core/defs.h : constant parameter definitions 
* src/core/wrappers.cpp : encoding/decoding subroutines including wavelet transform and range coding
* src/core/wrappers.h : header for wrappers.cpp
* src/generic/gen_aux.cpp : subroutines for handling generic Fortran/C/C++ compression control files
* src/generic/gen_aux.h : header for gen_aux.cpp
* src/generic/gen_dec.cpp : main generic Fortran/C/C++ file decoder program
* src/generic/gen_enc.cpp : main generic Fortran/C/C++ file encoder program
* src/flusi/hdf5_interfaces.cpp : subroutine for handling FluSI HDF5 files
* src/flusi/hdf5_interfaces.h : header for hdf5_interfaces.cpp
* src/flusi/main_dec.cpp : main FluSI decoder program
* src/flusi/main_enc.cpp : main FluSI encoder program
* src/mssg/ctrl_aux.cpp : subroutines for handling MSSG control files
* src/mssg/ctrl_aux.h : header for ctrl_aux.cpp
* src/mssg/mssg_dec.cpp : main MSSG decoder program
* src/mssg/mssg_enc.cpp : main MSSG encoder program
* src/rangecod/Makefile : range coder make file
* src/rangecod/port.h : range coder constant parameters
* src/rangecod/rangecod.cpp : range coder subroutines
* src/rangecod/rangecod.h : header for rangecod.cpp
* src/waveletcdf97_3d/Makefile : wavelet transform make file
* src/waveletcdf97_3d/waveletcdf97_3d.cpp : wavelet transform subroutines
* src/waveletcdf97_3d/waveletcdf97_3d.h : header for waveletcdf97_3d.cpp
* examples/generic/generic_enc_dec.sh : encode/decode sample bash script for generic Fortran/C/C++ files
* examples/generic/create_in_field.f90 : Fortran program to generate a sample data file
* examples/generic/Makefile : make file for create_in_field
* examples/generic/inmeta : example generic Fortran/C/C++ encoder parameter
* examples/generic/outmeta : example generic Fortran/C/C++ decoder parameters
* examples/generic/wrdec : symbolic link to WaveRange generic Fortran/C/C++ decoder executable
* examples/generic/wrenc : symbolic link to WaveRange generic Fortran/C/C++ encoder executable
* examples/flusi/flusi_dec_enc.sh : encode/decode sample bash script for FluSI
* examples/flusi/inmeta : example FluSI encoder parameter
* examples/flusi/outmeta : example FluSI decoder parameters
* examples/flusi/wrdec : symbolic link to WaveRange FluSI decoder executable
* examples/flusi/wrenc : symbolic link to WaveRange FluSI encoder executable
* examples/mssg/regout/all_enc_dec.sh : encode/decode sample bash script for MSSG regular output
* examples/mssg/regout/inmeta : example MSSG regular output encoder parameters
* examples/mssg/regout/outmeta : example MSSG regular output decoder parameters
* examples/mssg/regout/wrmssgdec : symbolic link to WaveRange MSSG decoder executable
* examples/mssg/regout/wrmssgenc : symbolic link to WaveRange MSSG encoder executable
* examples/mssg/divided/all_enc_dec.sh : encode/decode sample bash script for MSSG divided restart files
* examples/mssg/divided/wrmssgdec : symbolic link to WaveRange MSSG decoder executable
* examples/mssg/divided/wrmssgenc : symbolic link to WaveRange MSSG encoder executable
* examples/mssg/united/all_enc_dec.sh : encode/decode sample bash script for MSSG united restart files
* examples/mssg/united/wrmssgdec : symbolic link to WaveRange MSSG decoder executable
* examples/mssg/united/wrmssgenc :symbolic link to WaveRange MSSG encoder executable
* examples/fortran/example_fort.f90 : Fortran example of using WaveRange as a library
* examples/fortran/Makefile : make file for example_fort
* examples/fortran/libwaverange.so : symbolic link to WaveRange shared library

