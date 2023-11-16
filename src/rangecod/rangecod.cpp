/*
    rangecod.cpp : This file is part of WaveRange CFD data compression utility

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

    Modified from the rngcod13 coder (http://www.compressconsult.com/rangecoder/)
    Original header follows:

    rangecod.cpp     range encoding

    (c) Michael Schindler
    1997, 1998, 1999, 2000
    http://www.compressconsult.com/
    michael@compressconsult.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.  It may be that this
    program violates local patents in your country, however it is
    belived (NO WARRANTY!) to be patent-free. Glen Langdon also
    confirmed my poinion that IBM UK did not protect that method.


    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA 02111-1307, USA.

    Range encoding is based on an article by G.N.N. Martin, submitted
    March 1979 and presented on the Video & Data Recording Conference,
    Southampton, July 24-27, 1979. If anyone can name the original
    copyright holder of that article please contact me; this might
    allow me to make that article available on the net for general
    public.

    Range coding is closely related to arithmetic coding, except that
    it does renormalisation in larger units than bits and is thus
    faster. An earlier version of this code was distributed as byte
    oriented arithmetic coding, but then I had no knowledge of Martin's
    paper from 1997.

    The input and output is done by the inbyte and outbyte macros
    defined in the .c file; change them as needed; the first parameter
    passed to them is a pointer to the rangecoder structure; extend that
    structure as needed (and don't forget to initialize the values in
    start_encoding resp. start_decoding). This distribution writes to
    stdout and reads from stdin.

    There are no global or static var's, so if the IO is thread save the
    whole rangecoder is - unless GLOBALRANGECODER in rangecod.h is defined.

    For error recovery the last 3 bytes written contain the total number
    of bytes written since starting the encoder. This can be used to
    locate the beginning of a block if you have only the end. The header
    size you pass to initrangecoder is included in that count.

    There is a supplementary file called renorm95.c available at the
    website (www.compressconsult.com/rangecoder/) that changes the range
    coder to an arithmetic coder for speed comparisons.

    define RENORM95 if you want the arithmetic coder type renormalisation.
    Requires renorm95.c
    Note that the old version does not write out the bytes since init.
    you should not define GLOBALRANGECODER then. This Flag is provided
    only for spped comparisons between both renormalizations, see my
    data compression conference article 1998 for details.
*/

/*
  define NOWARN if you do not expect more than 2^32 outstanding bytes 
  since I recommend restarting the coder in intervals of less than    
  2^23 symbols for error tolerance this is not expected
*/
#define NOWARN

/*
  define EXTRAFAST for increased speed; you loose compression and
  compatibility in exchange.
*/
/* #define EXTRAFAST */

#include <stdio.h>		/* fprintf(), getchar(), putchar(), NULL */
#include <stdlib.h>              /* calloc() */
#include "port.h"
#include "rangecod.h"

/* SIZE OF RANGE ENCODING CODE VALUES. */

#define CODE_BITS 32
#define Top_value ((code_value)1 << (CODE_BITS-1))

#ifdef RENORM95
#include "renorm95.c"

#else
#define SHIFT_BITS (CODE_BITS - 9)
#define EXTRA_BITS ((CODE_BITS-2) % 8 + 1)
#define Bottom_value (Top_value >> 8)

#ifdef NOWARN
char coderversion[]="rangecoder 1.3 NOWARN (c) 1997-2000 Michael Schindler";
#else    /*NOWARN*/
char coderversion[]="rangecoder 1.3 (c) 1997-2000 Michael Schindler";
#endif   /*NOWARN*/
#endif   /*RENORM95*/


/* Read, echo and store a character from buf                 */  
static Inline unsigned char outbyte( rangecoder *rc, unsigned char buf )
{
#ifndef NOWARN
    if ((*rc).datapos >= (*rc).datalen) 
    {  printf( "Error: outbyte (*rc).datalen limit exceeded\n" );
       abort();
    }
#endif
    (*rc).databuf[(*rc).datapos++] = buf;
    return buf;
}


/* Return the next stored character                          */
static Inline unsigned char inbyte( rangecoder *rc )
{
#ifndef NOWARN
    if ((*rc).datapos >= (*rc).datalen)
    {  printf( "Error: inbyte (*rc).datalen limit exceeded\n" );
       abort();
    }
#endif
    return (*rc).databuf[(*rc).datapos++];
}


/* rc is the range coder to be used                            */
/* c is written as first byte in the datastream                */
/* one could do without c, but then you have an additional if  */
/* per outputbyte.                                             */
void start_encoding( rangecoder *rc, char c, unsigned long int initlength )
{   (*rc).low = 0;                /* Full code range */
    (*rc).range = Top_value;
    (*rc).buffer = c;
    (*rc).help = 0;               /* No bytes to follow */
    (*rc).bytecount = initlength;
}


#ifndef RENORM95
/* I do the normalization before I need a defined state instead of */
/* after messing it up. This simplifies starting and ending.       */
static Inline void enc_normalize( rangecoder *rc )
{   while((*rc).range <= Bottom_value)     /* do we need renormalisation?  */
    {   if ((*rc).low < (code_value)0xff<<SHIFT_BITS)  /* no carry possible --> output */
        {   outbyte(rc,(*rc).buffer);
            for(; (*rc).help; (*rc).help--)
                outbyte(rc,0xff);
            (*rc).buffer = (unsigned char)((*rc).low >> SHIFT_BITS);
        } else if ((*rc).low & Top_value) /* carry now, no future carry */
        {   outbyte(rc,(*rc).buffer+1);
            for(; (*rc).help; (*rc).help--)
                outbyte(rc,0);
            (*rc).buffer = (unsigned char)((*rc).low >> SHIFT_BITS);
        } else                           /* passes on a potential carry */
#ifdef NOWARN
            (*rc).help++;
#else
            if ((*rc).bytestofollow++ == 0xffffffffL)
            {   fprintf(stderr,"Too many bytes outstanding - File too large\n");
                exit(1);
            }
#endif
        (*rc).range <<= 8;
        (*rc).low = ((*rc).low<<8) & (Top_value-1);
        (*rc).bytecount++;
    }
}
#endif


/* Encode a symbol using frequencies                         */
/* rc is the range coder to be used                          */
/* sy_f is the interval length (frequency of the symbol)     */
/* lt_f is the lower end (frequency sum of < symbols)        */
/* tot_f is the total interval length (total frequency sum)  */
/* or (faster): tot_f = (code_value)1<<shift                             */
void encode_freq( rangecoder *rc, freq sy_f, freq lt_f, freq tot_f )
{	code_value r, tmp;
	enc_normalize( rc );
	r = (*rc).range / tot_f;
	tmp = r * lt_f;
	(*rc).low += tmp;
#ifdef EXTRAFAST
    (*rc).range = r * sy_f;
#else
    (*rc).range -= tmp;
    if (lt_f+sy_f < tot_f) (*rc).range = r * sy_f;
#endif
}

void encode_shift( rangecoder *rc, freq sy_f, freq lt_f, freq shift )
{	code_value r, tmp;
	enc_normalize( rc );
	r = (*rc).range >> shift;
	tmp = r * lt_f;
	(*rc).low += tmp;
#ifdef EXTRAFAST
	(*rc).range = r * sy_f;
#else
	if ((lt_f+sy_f) >> shift)
		(*rc).range -= tmp;
	else  
		(*rc).range = r * sy_f;
#endif
}


#ifndef RENORM95
/* Finish encoding                                           */
/* rc is the range coder to be used                          */
/* actually not that many bytes need to be output, but who   */
/* cares. I output them because decode will read them :)     */
/* the return value is the number of bytes written           */
uint4 done_encoding( rangecoder *rc )
{   uint tmp;
    enc_normalize(rc);     /* now we have a normalized state */
    (*rc).bytecount += 5;
    if (((*rc).low & (Bottom_value-1)) < (((*rc).bytecount&0xffffffL)>>1))
       tmp = (*rc).low >> SHIFT_BITS;
    else
       tmp = ((*rc).low >> SHIFT_BITS) + 1;
    if (tmp > 0xff) /* we have a carry */
    {   outbyte(rc,(*rc).buffer+1);
        for(; (*rc).help; (*rc).help--)
            outbyte(rc,0);
    } else  /* no carry */
    {   outbyte(rc,(*rc).buffer);
        for(; (*rc).help; (*rc).help--)
            outbyte(rc,0xff);
    }
    outbyte(rc,tmp & 0xff);
    outbyte(rc,((*rc).bytecount>>16) & 0xff);
    outbyte(rc,((*rc).bytecount>>8) & 0xff);
    outbyte(rc,(*rc).bytecount & 0xff);
    return (*rc).bytecount;
}


/* Start the decoder                                         */
/* rc is the range coder to be used                          */
/* returns the char from start_encoding or EOF               */
int start_decoding( rangecoder *rc )
{   int c = inbyte(rc);
    if (c==EOF)
        return EOF;
    (*rc).buffer = inbyte(rc);
    (*rc).low = (*rc).buffer >> (8-EXTRA_BITS);
    (*rc).range = (code_value)1 << EXTRA_BITS;
    return c;
}


static Inline void dec_normalize( rangecoder *rc )
{   while ((*rc).range <= Bottom_value)
    {   (*rc).low = ((*rc).low<<8) | (((*rc).buffer<<EXTRA_BITS)&0xff);
        (*rc).buffer = inbyte(rc);
        (*rc).low |= (*rc).buffer >> (8-EXTRA_BITS);
        (*rc).range <<= 8;
    }
}
#endif


/* Calculate culmulative frequency for next symbol. Does NO update!*/
/* rc is the range coder to be used                          */
/* tot_f is the total frequency                              */
/* or: totf is (code_value)1<<shift                                      */
/* returns the culmulative frequency                         */
freq decode_culfreq( rangecoder *rc, freq tot_f )
{   freq tmp;
    dec_normalize(rc);
    (*rc).help = (*rc).range/tot_f;
    tmp = (*rc).low/(*rc).help;
#ifdef EXTRAFAST
    return tmp;
#else
    return (tmp>=tot_f ? tot_f-1 : tmp);
#endif
}

freq decode_culshift( rangecoder *rc, freq shift )
{   freq tmp;
    dec_normalize(rc);
    (*rc).help = (*rc).range>>shift;
    tmp = (*rc).low/(*rc).help;
#ifdef EXTRAFAST
    return tmp;
#else
    return (tmp>>shift ? ((code_value)1<<shift)-1 : tmp);
#endif
}


/* Update decoding state                                     */
/* rc is the range coder to be used                          */
/* sy_f is the interval length (frequency of the symbol)     */
/* lt_f is the lower end (frequency sum of < symbols)        */
/* tot_f is the total interval length (total frequency sum)  */
void decode_update( rangecoder *rc, freq sy_f, freq lt_f, freq tot_f)
{   code_value tmp;
    tmp = (*rc).help * lt_f;
    (*rc).low -= tmp;
#ifdef EXTRAFAST
    (*rc).range = (*rc).help * sy_f;
#else
    if (lt_f + sy_f < tot_f)
        (*rc).range = (*rc).help * sy_f;
    else
        (*rc).range -= tmp;
#endif
}


/* Decode a byte/short without modelling                     */
/* rc is the range coder to be used                          */
unsigned char decode_byte(rangecoder *rc)
{   unsigned char tmp = decode_culshift(rc,8);
    decode_update( rc,1,tmp,(freq)1<<8);
    return tmp;
}

unsigned short decode_short(rangecoder *rc)
{   unsigned short tmp = decode_culshift(rc,16);
    decode_update( rc,1,tmp,(freq)1<<16);
    return tmp;
}


/* Finish decoding                                           */
/* rc is the range coder to be used                          */
void done_decoding( rangecoder *rc )
{   dec_normalize(rc);      /* normalize to use up all bytes */
}


/* Set up the data buffer array                              */
void init_databuf( rangecoder *rc, unsigned long int maxlen )
{   (*rc).datalen = maxlen;
    (*rc).datapos = 0;
    (*rc).databuf = (unsigned char *)calloc(maxlen, sizeof(unsigned char) );
}

/* Deallocate the data buffer array                          */
void free_databuf( rangecoder *rc )
{   free((*rc).databuf);
}

/* Count number of occurances of each byte */
void countblock(int *buffer, freq length, freq *counters)
{   int i;
    /* First zero the counters */
    for (i=0; i<257; i++)
        counters[i] = 0;
    /* Then count the number of occurrence of each byte */
    for (i=0; i<length; i++)
        counters[buffer[i]]++;
}

/* Decode the frequencies */
void readcounts(rangecoder *rc, freq *counters)
{   freq i;
    for (i=0; i<256; i++)
        counters[i] = decode_short(rc);
}
