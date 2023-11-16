/*
    wrappers.cpp : This file is part of WaveRange CFD data compression utility

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
  
    Reference:
    doc/cfdproc2017.pdf
    Dmitry Kolomenskiy, Ryo Onishi and Hitoshi Uehara "Wavelet-Based Compression of CFD Big Data"
    Proceedings of the 31st Computational Fluid Dynamics Symposium, Kyoto, December 12-14, 2017
    Paper No. C08-1

    This work is supported by the FLAGSHIP2020, MEXT within the priority study4 
    (Advancement of meteorological and global environmental predictions utilizing 
    observational “Big Data”).
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

#include <iostream>
#include <exception>
#include <memory>

#include "../rangecod/port.h"
#include "../rangecod/rangecod.h"
#include "../waveletcdf97_3d/waveletcdf97_3d.h"

#include "../core/defs.h"
#include "wrappers.h"

using namespace std;


/* Calculate local precision */
template <typename T>
static T lcl_prec(int nx, int ny, int nz, int jx, int jy, int jz, int mx, int my, int mz, T *cutoffvec)
{
    // Cartesian coordinates of the local precision cutoff block
    int kx = int(T(jx)/T(nx)*T(mx));
    int ky = int(T(jy)/T(ny)*T(my));
    int kz = int(T(jz)/T(nz)*T(mz));

    // Evaluate the local precision
    return cutoffvec[kx+mx*ky+mx*my*kz];
}


/* Use the range encoder to code an array fld_q */ 
static void range_encode(unsigned char *fld_q, unsigned long int ntot, unsigned char *enc_q, unsigned long int& len_out_q)
{   freq counts[257], blocksize, i;
    int buffer[BLOCKSIZE];
    unsigned long int j;
    unsigned long int pos_in = 0;
    unsigned long int pos_out = 0;

    // Allocate a range coder object
    rangecoder *rc = (rangecoder*)malloc(sizeof(rangecoder));

    // Initialize data buffer
    init_databuf(rc,2*BLOCKSIZE+1000);

    // Start up the range coder, first byte 0, no header
    start_encoding(rc,0,0);

    // Coding: loop for all blocks of the input vector
    while (1)
    {
        // Put data in a buffer
        for (blocksize = 0; blocksize < BLOCKSIZE; blocksize++) {
          buffer[blocksize] = fld_q[pos_in];
          pos_in++; 
          if (pos_in>ntot) break;
        }

        // Block start marker
        encode_freq(rc,1,1,2);

        // Get the statistics 
        countblock(buffer,blocksize,counts);

        // Write the statistics.
        // Cant use putchar or other since we are after start of the rangecoder 
        // as you can see the rangecoder doesn't care where probabilities come 
        // from, it uses a flat distribution of 0..0xffff in encode_short. 
        for(i=0; i<256; i++)
            encode_short(rc,counts[i]);

        // Store in counters[i] the number of all bytes < i, so sum up 
        counts[256] = blocksize;
        for (i=256; i; i--)
            counts[i-1] = counts[i]-counts[i-1];

        // Output the encoded symbols 
        for(i=0; i<blocksize; i++) {
            int ch = buffer[i];
            encode_freq(rc,counts[ch+1]-counts[ch],counts[ch],counts[256]);
        }

        // Copy data from a buffer to the output vector
        for (j = 0; j < rc->datapos; j++) {
          enc_q[pos_out++] = rc->databuf[j];
        }

        // Reset the buffer    
        rc->datapos = 0;

        // Terminate if no more data
        if (blocksize<BLOCKSIZE) break;
    }

    // Flag absence of next block by a bit 
    encode_freq(rc,1,0,2);

    // Finalize the encoder 
    done_encoding(rc);

    // Copy data from a buffer to the output vector
    for (j = 0; j < rc->datapos; j++) {
      enc_q[pos_out++] = rc->databuf[j];
    }

    // True length of the encoded array
    len_out_q = pos_out;

    // Deallocate data buffer
    free_databuf(rc);

    // Deallocate range coder object
    free(rc);
}


/* Decode the range-encoded data enc_q */
static void range_decode(unsigned char *enc_q, unsigned long int len_out_q, unsigned char *dec_q, unsigned long int ntot)
{   freq counts[257], blocksize, i, i1, cf, symbol, middle;

    // Allocate a range coder object
    rangecoder *rc = (rangecoder*)malloc(sizeof(rangecoder));

    // Initialize data buffer
    rc->help = 0;
    unsigned long int pos_out = 0;
    init_databuf(rc,len_out_q);
    for(unsigned long int j = 0; j < len_out_q; j++) rc->databuf[j] = enc_q[j];
    rc->datalen = len_out_q;
    rc->datapos = 0;

    // Start decoding
    if (start_decoding(rc) != 0)
    {   fprintf(stderr,"could not successfully open input data\n");
        exit(1);
    }

    // Decoding: loop for all blocks of the input vector
    while (cf = decode_culfreq(rc,2))
    {   // Read the beginning of the block
        decode_update(rc,1,1,2);

        // Read frequencies
        readcounts(rc,counts);

        // Figure out blocksize by summing counts; also use counts as in encoder 
        blocksize = 0;
        for (i=0; i<256; i++)
        {   freq tmp = counts[i];
            counts[i] = blocksize;
            blocksize += tmp;
        }
        counts[256] = blocksize;

        // Allocate search table
        freq *invcounts = new freq[blocksize+1];

        // Fill values in the search table
        for (i1=0; i1<256; i1++) 
            for (i=counts[i1]; i<counts[i1+1]; i++) invcounts[i] = i1;
        invcounts[blocksize] = 256;

        // Loop for all decoding elements in the block
        for (i=0; i<blocksize; i++)
        {   // Decode frequency
            cf = decode_culfreq(rc,blocksize);
            // Symbol from the search table 
            middle = invcounts[cf];
            // If some symbols have zero frequency, skip them
            for (symbol=middle; counts[symbol+1]<=cf; symbol++);
            // Update the decoder
            decode_update(rc, counts[symbol+1]-counts[symbol],counts[symbol],blocksize);
            // Store the decoded element
            dec_q[pos_out++] = symbol;
        }

        // Delete search table
        delete [] invcounts;
    }

    // Finalize decoding 
    done_decoding(rc);

    // Deallocate data buffer
    free_databuf(rc);

    // Deallocate range coder object
    free(rc);
}

extern "C" void encoding_wrap_float(int nx, int ny, int nz, float *fld_1d, int wtflag, int mx, int my, int mz, float *cutoffvec, float& tolabs, float& midval, float& halfspanval, unsigned char& wlev, unsigned char& nlay, unsigned long int& ntot_enc, float *deps_vec, float *minval_vec, unsigned long int *len_enc_vec, unsigned char *data_enc)
{
    encoding_wrap<float>(nx, ny, nz, fld_1d, wtflag, mx, my, mz, cutoffvec, tolabs, midval, halfspanval, wlev, nlay, ntot_enc, deps_vec, minval_vec, len_enc_vec, data_enc);
}

extern "C" void encoding_wrap_double(int nx, int ny, int nz, double *fld_1d, int wtflag, int mx, int my, int mz, double *cutoffvec, double& tolabs, double& midval, double& halfspanval, unsigned char& wlev, unsigned char& nlay, unsigned long int& ntot_enc, double *deps_vec, double *minval_vec, unsigned long int *len_enc_vec, unsigned char *data_enc)
{
    encoding_wrap<double>(nx, ny, nz, fld_1d, wtflag, mx, my, mz, cutoffvec, tolabs, midval, halfspanval, wlev, nlay, ntot_enc, deps_vec, minval_vec, len_enc_vec, data_enc);
}

extern "C" void decoding_wrap_float(int nx, int ny, int nz, float *fld_1d, float& tolabs, float& midval, float& halfspanval, unsigned char& wlev, unsigned char& nlay, unsigned long int& ntot_enc, float *deps_vec, float *minval_vec, unsigned long int *len_enc_vec, unsigned char *data_enc)
{
    decoding_wrap<float>(nx, ny, nz, fld_1d, tolabs, midval, halfspanval, wlev, nlay, ntot_enc, deps_vec, minval_vec, len_enc_vec, data_enc);
}
extern "C" void decoding_wrap_double(int nx, int ny, int nz, double *fld_1d, double& tolabs, double& midval, double& halfspanval, unsigned char& wlev, unsigned char& nlay, unsigned long int& ntot_enc, double *deps_vec, double *minval_vec, unsigned long int *len_enc_vec, unsigned char *data_enc)
{
    decoding_wrap<double>(nx, ny, nz, fld_1d, tolabs, midval, halfspanval, wlev, nlay, ntot_enc, deps_vec, minval_vec, len_enc_vec, data_enc);
}


/* Encoding subroutine with wavelet transform and range coding */ 
template <typename T>
void encoding_wrap(int nx, int ny, int nz, T *fld_1d, int wtflag, int mx, int my, int mz, T *cutoffvec, T& tolabs, T& midval, T& halfspanval, unsigned char& wlev, unsigned char& nlay, unsigned long int& ntot_enc, T *deps_vec, T *minval_vec, unsigned long int *len_enc_vec, unsigned char *data_enc)
{
    /* Wavelet decomposition */

    // Total number of elements in the input array
    unsigned long int ntot = (unsigned long int)(nx)*(unsigned long int)(ny)*(unsigned long int)(nz);

    // Number of elements in the local cutoff array
    unsigned int mtot = mx*my*mz;

    // Wavelet transform depth, hardcoded, see header file; zero if no transform required
    if (wtflag) wlev = WAV_LVL; else wlev = 0;

    // Find the minimum and maximum values
    T minval = fld_1d[0];
    T maxval = fld_1d[0];
    for (unsigned long int j = 0; j < ntot; j++) 
      {
        minval = fmin(minval,fld_1d[j]);
        maxval = fmax(maxval,fld_1d[j]);
      }

    // Find the middle value and the half-span of the data values
    halfspanval = (maxval-minval)/2;
    midval = minval+halfspanval;

    // If the half-span is close to zero, encoding is impossible and unnecessary
    if (halfspanval <= 2*DBL_MIN)
      {
        // Encoded data array is empty and not used
        ntot_enc = 0;
        nlay = 0;
        tolabs = 0;

        // Exit from the subroutine
        return;
      }

    // Apply wavelet transform
    waveletcdf97_3d<T>(nx,ny,nz,int(wlev),fld_1d);

    /* Range encoding */

    // Alphabet size
    int q = 256;

    // Allocate the quantized input and output vectors
    unsigned char *fld_q = new unsigned char[ntot];
    unsigned char *enc_q = new unsigned char[(SAFETY_BUFFER_FACTOR+1UL)*(ntot<1024UL?1024UL:ntot)]; // Encoded array may be longer than the original

    // Output quantized data array length
    unsigned long int len_out_q = 0;

    // Output vector counter
    unsigned long int jtot = 0;

    // Byte layer counter
    unsigned char ilay = 0;

    // Minimum cutoff
    T tolrel = cutoffvec[0];
    for (unsigned int k=1; k<mtot; k++) if (cutoffvec[k] < tolrel) tolrel = cutoffvec[k];

    // Absolute tolerance
    tolabs = tolrel * fmax(fabs(minval),fabs(maxval));

    // Apply a correction for round-off errors in wavelet transform
    tolabs /= WAV_ACC_COEF;

    // Iteration break flag set to false by default
    unsigned char brflag = 0;

    // Quantize and encode all byte layers
    while (1)
    {
        // Calculate min and max
        minval = fld_1d[0];
        maxval = fld_1d[0];
        for(unsigned long int j = 1; j < ntot; j++) 
          {
            minval = fmin(minval,fld_1d[j]);
            maxval = fmax(maxval,fld_1d[j]);
          }

        // Store the minimum value
        minval_vec[ilay] = minval;


        // Quantize for a q-letter alphabet
        T deps = (maxval-minval)/(T)(q-1);

        // Impose the desired accuracy of the least significant bit plane
        if (deps < tolabs) 
        {
          deps = tolabs;
          brflag = 1;
        }

        // Termnate if maximum bit plane is reached
        if (ilay >= NLAYMAX-1U) brflag = 1;

        // Save the quantization interval size
        deps_vec[ilay] = deps;

        // Combinations of minval and deps, for optimization
        T aopt = 1.0/deps;
        T bopt = -minval*aopt+0.5;

        // Two branches depending on the local precision mask activated or not
        if (mtot > 1)
          {
          // Loop for all jp in physical space
          for(unsigned long int jp = 0; jp < ntot; jp++)
            {   
              // Calculate 3D index in wavelet space
              int l, jwx, jwy, jwz;
              ind_p2w_3d( wlev, nx, ny, nz, jp%nx, (jp/nx)%ny, jp/nx/ny, &l, &jwx, &jwy, &jwz);

              // Uniform cutoff by default
              T precmask = tolabs;

              // Load precision cutoff function in physical space, only applied on smallest detail coefficients
              if (l <= LOC_CUTOFF_LVL) 
                precmask = tolabs/tolrel * lcl_prec(nx, ny, nz, jp%nx, (jp/nx)%ny, jp/nx/ny, mx, my, mz, cutoffvec);

              // 1D index in wavelet space
              unsigned long int jw = (unsigned long int)(jwx) + 
                                     (unsigned long int)(nx)*(unsigned long int)(jwy) +
                                     (unsigned long int)(nx)*(unsigned long int)(ny)*(unsigned long int)(jwz);
  
              // For every element, quantize depending on the local precision mask 
              if (maxval-minval < precmask) 
                { 
                  // Set to zero if the full range is less than the local precision
                  // Note that fld_q is in wavelet space
                  fld_q[jw] = 0;
                  fld_1d[jw] = minval; 
                } 
              else 
                { 
                  // Set to a value between 0 and 256 if the full range is greater than the local precision
                  T fq = aopt * fld_1d[jw] + bopt; // fld_1d[jp]-minval is always >= 0
                  fld_q[jw] = fq;
                }
            }
          }
        // If the local precision mask is not activated, use faster loop
        else
          {
          // Loop for all jp in physical space
          for(unsigned long int jp = 0; jp < ntot; jp++)
            {   
              // Set to a value between 0 and 256 if the full range is greater than the local precision
              T fq = aopt * fld_1d[jp] + bopt; // fld_1d[jp]-minval is always >= 0
              fld_q[jp] = fq;
            }
          }   	 
    
        // The following only executes if all quantized data are non-zero

        // Residual field
        for(unsigned long int j = 0; j < ntot; j++)
          fld_1d[j] = fld_1d[j] - ( fld_q[j]*deps + minval );

        // Check quantized data bounds
        unsigned char iminval = fld_q[0];
        unsigned char imaxval = fld_q[0];
        for(unsigned long int j = 1; j < ntot; j++)
          {
             unsigned char fld1_q_tmp = fld_q[j];
             if (fld1_q_tmp<iminval) iminval = fld1_q_tmp;
             if (fld1_q_tmp>imaxval) imaxval = fld1_q_tmp;
          }

        // Encode
        range_encode(fld_q,ntot,enc_q,len_out_q);

        // Store the encoded data
        len_enc_vec[ilay] = len_out_q;
        for(unsigned long int j = 0; j < len_out_q; j++) 
          {
            // Store the encoded data element
            data_enc[jtot++] = enc_q[j];

            // Check for overflow
            if (jtot > SAFETY_BUFFER_FACTOR*NLAYMAX*(ntot<1024UL?1024UL:ntot))
              {
                cout << "Error: encoded array is too large. Use larger SAFETY_BUFFER_FACTOR" << endl;
                throw std::exception();
              }
          }


        // Update layer index
        ilay ++;

        // Stop when the required tolerance is reached for all samples
        if ( brflag ) 
          {
             // Stop iterations
             break;
          }
    }

    // Number of byte layers
    nlay = ilay;

    // Total encoded data array length
    ntot_enc = jtot;

    // Deallocate memory
    delete [] fld_q;
    delete [] enc_q;
}


/* Decoding subroutine with range decoding and inverse wavelet transform*/
template <typename T>
void decoding_wrap(int nx, int ny, int nz, T *fld_1d, T& tolabs, T& midval, T& halfspanval, unsigned char& wlev, unsigned char& nlay, unsigned long int& ntot_enc, T *deps_vec, T *minval_vec, unsigned long int *len_enc_vec, unsigned char *data_enc)
{
    // Total number of elements
    unsigned long int ntot = (unsigned long int)(nx)*(unsigned long int)(ny)*(unsigned long int)(nz);

    // Case of trivial data
    if (ntot_enc == 0)
      {
        // Reconstruct
        for(unsigned long int j = 0; j < ntot; j++) fld_1d[j] = midval;

        // Exit 
        return;
      }

    /* Range decoding */

    // Allocate the quantized input and output vectors
    unsigned char *dec_q = new unsigned char[ntot];
    unsigned char *enc_q = new unsigned char[(SAFETY_BUFFER_FACTOR+1UL)*(ntot<1024UL?1024UL:ntot)]; // Encoded array may be longer than the original

    // Cumulative field
    for(unsigned long int j = 0; j < ntot; j++) fld_1d[j] = 0;

    // Input vector counter
    unsigned long int jtot = 0;

    // Output the decoded output vector   
    for (unsigned char ilay = 0; ilay < nlay; ilay++)
    {

        // Parameters for reconstruction
        T deps = deps_vec[ilay];
        T minval = minval_vec[ilay];

        // Get the encoded data
        unsigned long int len_out_q = len_enc_vec[ilay];
        for(unsigned long int j = 0; j < len_out_q; j++) enc_q[j] = data_enc[jtot++];

        // Decode
        range_decode(enc_q,len_out_q,dec_q,ntot);

        // Check quantized data bounds
        unsigned char iminval = dec_q[0];
        unsigned char imaxval = dec_q[0];
        for(unsigned long int j = 1; j < ntot; j++)
          {
             if (dec_q[j]<iminval) iminval = dec_q[j];
             if (dec_q[j]>imaxval) imaxval = dec_q[j];
          }

        // Cumulative field
        for(unsigned long int j = 0; j < ntot; j++)
          fld_1d[j] = fld_1d[j] + ( dec_q[j]*deps + minval );
    }

    /* Wavelet reconstruction */

    // Inverse wavelet transform if the data is non-trivial
    waveletcdf97_3d(nx,ny,nz,-int(wlev),fld_1d);

    // Deallocate memory
    delete [] enc_q;
    delete [] dec_q;
}


/* Return the number of bit planes and the required encoded data array size */ 
extern "C" void setup_wr(int nx, int ny, int nz, unsigned char& nlaymax, unsigned long int& ntot_enc_max)
{
    // Number of bit planes
    nlaymax = NLAYMAX;

    // Total number of elements in the input array
    unsigned long int ntot = (unsigned long int)(nx)*(unsigned long int)(ny)*(unsigned long int)(nz);

    // Required encoded data array size, for memory allocation
    ntot_enc_max = SAFETY_BUFFER_FACTOR*NLAYMAX*(ntot<1024UL?1024UL:ntot);
}


/* Fortran interface. Encoding subroutine with wavelet transform and range coding */ 
extern "C" void encoding_wrap_f(int *nx, int *ny, int *nz, double *fld, int *wtflag, double *tolrel, double& tolabs, double& midval, double& halfspanval, unsigned char& wlev, unsigned char& nlay, long int& ntot_enc_sg, double *deps_vec, double *minval_vec, long int *len_enc_vec_sg, unsigned char *data_enc)
{
    // Unsigned long int variables
    unsigned long int ntot_enc;
    unsigned long int len_enc_vec[NLAYMAX];

    // Uniform local cutoff tolerance
    int mx = 1, my = 1, mz = 1;
    double *cutoffvec = new double[1];
    cutoffvec[0] = *tolrel;

    // Apply encoding routine
    encoding_wrap(*nx,*ny,*nz,fld,*wtflag,mx,my,mz,cutoffvec,tolabs,midval,halfspanval,wlev,nlay,ntot_enc,deps_vec,minval_vec,len_enc_vec,data_enc);

    // Standard Fortran does not have an equivalent to unsigned long int, therefore, we copy the values into signed variables hoping that they fit in
    ntot_enc_sg = (long int)(ntot_enc);
    for (unsigned char j = 0; j < NLAYMAX; j++) 
      len_enc_vec_sg[j] = (long int)(len_enc_vec[j]);
}


/* Fortran interface. Decoding subroutine with range decoding and inverse wavelet transform */ 
extern "C" void decoding_wrap_f(int *nx, int *ny, int *nz, double *fld, double& midval, double& halfspanval, unsigned char& wlev, unsigned char& nlay, long int& ntot_enc_sg, double *deps_vec, double *minval_vec, long int *len_enc_vec_sg, unsigned char *data_enc)
{
    // The variable tolabs is not used, but it is required in the C interface for backward compatibility
    double tolabs = 0;

    // Unsigned long int variables
    unsigned long int ntot_enc = (unsigned long int)(ntot_enc_sg);
    unsigned long int len_enc_vec[NLAYMAX];
    for (unsigned char j = 0; j < NLAYMAX; j++) len_enc_vec[j] = 
      (unsigned long int)(len_enc_vec_sg[j]);

    // Apply decoding routine
    decoding_wrap(*nx,*ny,*nz,fld,tolabs,midval,halfspanval,wlev,nlay,ntot_enc,deps_vec,minval_vec,len_enc_vec,data_enc);
}


/* Fortran interface. Return the number of bit planes and the required encoded data array size */ 
extern "C" void setup_wr_f(int *nx, int *ny, int *nz, int& nlaymax, long int& ntot_enc_max)
{
    // Number of bit planes
    nlaymax = NLAYMAX;

    // Total number of elements in the input array
    long int ntot = (long int)(*nx)*(long int)(*ny)*(long int)(*nz);

    // Required encoded data array size, for memory allocation
    ntot_enc_max = SAFETY_BUFFER_FACTOR*NLAYMAX*(ntot<1024L?1024L:ntot);
}

