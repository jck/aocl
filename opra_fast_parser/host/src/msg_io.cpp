// Copyright (C) 2013-2014 Altera Corporation, San Jose, California, USA. All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to
// whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
// 
// This agreement shall be governed in all respects by the laws of the State of California and
// by the laws of the United States of America.

// Message Writing Functions
// 
// Copyright 2012 Altera Corporation.

#include "msg_io.h"
#include "decode_types.h"
#include "msg_types.h"

#ifdef LINUX
#ifdef USE_NTOHL
#include <arpa/inet.h>
#endif
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>


// This file contains helper functions for parsing and outputting OPRA data


static int64_t l_fields[DEF_MSG_V2+1];
void * l_ctx = NULL;
void * l_link = NULL;

static int l_invalid_count = 0;


static void 
l_encode_field( unsigned char * buffer, int mult, uint64_t *scratch, uint64_t *decomp ) 
{
    unsigned char j;

    (*scratch) = ( buffer[1] ) | 
        ( (uint64_t)buffer[2] << 8  ) | 
        ( (uint64_t)buffer[3] << 16 ) | 
        ( (uint64_t)buffer[4] << 24 ) | 
        ( (uint64_t)buffer[5] << 32 ) | 
        ( (uint64_t)buffer[6] << 40 ) | 
        ( (uint64_t)buffer[7] << 48 ) ;

    *decomp = 0;
    for (j=0;j<7;j++) {
        if (buffer[0] >= ((j+1)*mult) ) {
            (*decomp) |= ( ( (*scratch) & (((uint64_t)0x7f) << (7*j) ) ) << j ) ;
        } 
    }
    (*decomp) |= 0x80;
}

static void 
l_extract_field( unsigned char * buffer, uint64_t *field, unsigned char *field_id, uint64_t *scratch, unsigned char n_groups ) 
{
    (*scratch) = ( buffer[1] ) | 
        ( (uint64_t)buffer[2] << 8  ) | 
        ( (uint64_t)buffer[3] << 16 ) | 
        ( (uint64_t)buffer[4] << 24 ) | 
        ( (uint64_t)buffer[5] << 32 ) | 
        ( (uint64_t)buffer[6] << 40 ) | 
        ( (uint64_t)buffer[7] << 48 ) ;
    (*field) = (*scratch) & (  ((uint64_t)0xffffffffffffffff ) >> (64-(n_groups*7)) );

    (*field_id) = buffer[0];

}

static void 
l_dump_security_symbol( uint64_t field, char ** buf ) 
{
    int i;
    uint64_t field_val = field;
#ifdef COLOUR_MARK
    if (fp == stdout ) {
        fprintf(fp,"\033[22;34m");
    }
#endif
    for (i=0;i<SECURITY_SYMBOL_SIZE;i++) {
        if ( *buf ) {
            (*buf)[0] = ( field_val >> ((SECURITY_SYMBOL_SIZE-1-i)*7) ) & 0x7f;
            (*buf) ++;
        }
    }
#ifdef COLOUR_MARK
    if (fp == stdout ) {
        fprintf(fp,"\033[22;30m");
    }
#endif
}

static int 
l_extract_as_int( uint64_t field, int size ) 
{
    uint64_t mask = ( ((uint64_t)1) << (size*7) ) - 1;
    return (int)(field & mask);
}

static const char * 
l_convert_put_call( char code ) 
{
    switch (code) {
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
            return "call";
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
            return "put";
    }
#ifdef DUMP_ERRORS
    fprintf(stderr,"ERROR: INVALID P/C\n");
#endif
    l_invalid_count++;

    return "inv";
}

static const char * 
l_convert_month( char code ) 
{
    switch (code) {
        case 'A':
        case 'M':
            return "JAN";
        case 'B':
        case 'N':
            return "FEB";
        case 'C':
        case 'O':
            return "MAR";
        case 'D':
        case 'P':
            return "APR";
        case 'E':
        case 'Q':
            return "MAY";
        case 'F':
        case 'R':
            return "JUN";
        case 'G':
        case 'S':
            return "JUL";
        case 'H':
        case 'T':
            return "AUG";
        case 'I':
        case 'U':
            return "SEP";
        case 'J':
        case 'V':
            return "OCT";
        case 'K':
        case 'W':
            return "NOV";
        case 'L':
        case 'X':
            return "DEC";
    }
#ifdef DUMP_ERRORS
    fprintf(stderr,"ERROR: INVALID MONTH\n");
#endif
    l_invalid_count++;

    return "INV";
}

static void 
l_dump_price( char denom_code, int64_t val, char** buf ) 
{
    int n_zeros = (denom_code - 'A') + 1;
    int denom = 1;

    if (n_zeros <= 0 ) {
#ifdef DUMP_ERRORS
        printf(" INVALID DENOM %X, %d ",denom_code, n_zeros );
#endif
        l_invalid_count++;
    }
    while (n_zeros>0) {
        denom = denom * 10;
        n_zeros--;
    }

    if (*buf) {
        if (denom <= 1 ) {
            sprintf(*buf, "%10.2f", 0.0);
        } else {
            sprintf(*buf, "%10.2f", ((double)val)/(double)denom );
        }
        (*buf) += 10;
    }

    // fprintf(fp,"%X, %d\n", val, val );
}


void init_screen_dump()
{
    memset( l_fields, 0, sizeof( l_fields ) );
}

void dump_decoded_msg( FILE*fp, ulong * h_verifyData, int messages, dump_t dump_type ) {
   for (int i = 0; i < messages; i++) {
      uint64_t *l_fields = (uint64_t *)(h_verifyData + i * FIELDS); 
      if ( 1/*l_fields[MESSAGE_CATEGORY_V2] == 'k'*/) {
         char buffer[1024];
         char *buf_ptr = buffer;

         l_dump_security_symbol( l_fields[SECURITY_SYMBOL_V2], &buf_ptr );
         buf_ptr[0] = '|';
         buf_ptr++;

         l_dump_price( l_fields[PREMIUM_PRICE_DENOMINATOR_CODE_V2],
                l_fields[BID_PRICE_V2], &buf_ptr );
 
         buf_ptr[0] = '|';
         buf_ptr ++ ;

         sprintf(buf_ptr,"%4d", l_fields[BID_SIZE_V2] );
         buf_ptr += 4;

         buf_ptr[0] = '|';
         buf_ptr ++ ;
 
         l_dump_price( l_fields[PREMIUM_PRICE_DENOMINATOR_CODE_V2],
                l_fields[OFFER_PRICE_V2], &buf_ptr );
         buf_ptr[0] = '|';
         buf_ptr ++ ;

         sprintf(buf_ptr,"%4d", l_fields[OFFER_SIZE_V2] );
         buf_ptr += 4;
         buf_ptr[0] = '|';
         buf_ptr++;

         l_dump_price( l_fields[STRIKE_PRICE_DENOMINATOR_CODE_V2],
                l_fields[EXPLICIT_STRIKE_PRICE_V2], &buf_ptr );

         sprintf( buf_ptr, "| %s-%02d-%02d ",
                l_convert_month( l_fields[EXPIRATION_MONTH_V2] ) ,
                l_fields[EXPIRATION_DATE_V2] ,
                l_fields[YEAR_V2]
               );
         buf_ptr = buffer + strlen(buffer);

         sprintf(buf_ptr, "| %s", l_convert_put_call( l_fields[EXPIRATION_MONTH_V2] ) );

         fprintf(fp,"%s\n", buffer);
      }
   }
}

int l_buffer_to_nmsg( unsigned char* buffer ) {
    int i;
    int val = 0;
    for (i=0;i<NMSG_SIZE;i++) {
        val *= 10;
        val += (unsigned int) ( buffer[NMSG_START_BYTE+i] - '0' );
    }
    return val;
}

void dump_raw_input( FILE*fp, int64_t* buffer, int nelems ) 
{
    int i;
    i = 0;
    int iframe = 0;
    while ( i < nelems ) {
        if ( i%sizeof(buffer[0]) == 0 ) {
            fprintf(fp,"%08d: ", i*sizeof(buffer[0]) );
        }
        fprintf(fp, "%016llX ", buffer[i++] );
        if (i && (i%(sizeof(buffer[0])/2) == 0) ) {
            fprintf(fp," ");
        }
        if (i && (i%sizeof(buffer[0]) == 0) ) {
            fprintf(fp,"\n");
        }
    }
}

void dump_byte_raw_input( char*fname, unsigned char * buffer, int nelems )
{
    FILE*fp;
    int i;
    if (fname == NULL ) {
        fp = stderr;
    } else {
        fp = fopen(fname,"w");
        assert(fp);
    }
    i = 0;
    int iframe = 0;
    while ( i < nelems ) {
        fprintf(fp, "%02X", buffer[i++] );
        if (i && (i%4 == 0) ) {
            fprintf(fp," ");
        }
        if (i && (i%8 == 0) ) {
            fprintf(fp,"\n");
        }
    }
    if (fname != NULL ) {
        fclose(fp);
    }
}

void pack_char_into_intbuf( unsigned char* buffer, int nelems, unsigned int * intbuf, int n_int_elems )
{
    int i;
    assert(nelems % sizeof(intbuf[0]) == 0);
    for (i=0;i<nelems;) {
        int j;
        assert( (i/sizeof(intbuf[0])) < n_int_elems );
        intbuf[i/sizeof(intbuf[0])] = 0;
        for (j=0;j<sizeof(intbuf[0]);j++) {
            intbuf[i/sizeof(intbuf[0])] |= (  ( 0xff & buffer[i] ) << (j*8) );
            i++;
            assert(i<=nelems);
        }
    }
}

int create_byte_msg(unsigned char msg_size, unsigned char* buffer, int nelems )
{
    int nbytes;
    buffer[0] = msg_size;
    for (nbytes=0;nbytes<msg_size;nbytes++) {
        buffer[nbytes+1] = 0xA5;
    }
    assert( nelems >= nbytes+1 );
    return nbytes+1;
}

int dump_simple( FILE*fp, unsigned char * kfields ) {
    int i;
    int offset = 0;

    fprintf(fp,"symbol: ");
    for (i=0;i<1;i++) {
        fprintf(fp,"%c",kfields[offset + i]);
    }
    offset += i;
    fprintf(fp,"\n");


    fprintf(fp,"Bid Price: %d\n", kfields[offset+0] | kfields[offset+1] << 8 | kfields[offset+2] << 16 );
    offset+= 3;
    fprintf(fp,"Offer Price: %d\n", kfields[offset+0] | kfields[offset+1] << 8 | kfields[offset+2] << 16 );
    offset+=3;

    return offset;
}

int dump_header( FILE*fp, unsigned char * hdr )
{
    int i;
    int offset = 0;
    fprintf(fp," msg type:       %02X  '%c'\n", hdr[offset], hdr[offset] );
    offset++;
    fprintf(fp," participant id: %02X  '%c'\n", hdr[offset], hdr[offset] );
    offset++;
    fprintf(fp," req id:         %02X  '%c'\n", hdr[offset], hdr[offset] );
    offset++;

    fprintf(fp," seq id: " );
    for (i=0;i<8;i++) {
        fprintf(fp,"%02X:", hdr[offset+7-i] );
    }
    offset+=i;
    fprintf(fp, "\n") ;

    fprintf(fp," time: " );
    for (i=0;i<4;i++) {
        fprintf(fp,"%02X:", hdr[offset+3-i] );
    }
    fprintf(fp,"\n");
    offset+=i;

    return offset;
}

void dump_raw_words( FILE*fp, unsigned char * buffer, int nelems ) 
{
    for (int i=0;i<nelems;i++) {
        if ( (i%8) == 0 ) {
            fprintf(fp," ");
        }
        if ( (i%32) == 0 ) {
            fprintf(fp,"\n");
        }

        fprintf(fp,"%02X",buffer[i] );
    }
    fprintf(fp,"\n");

}

void dump_decoded_word( FILE*fp, unsigned char * buffer, int nelems ) 
{
    int i = 0;
    int imsg = 0;
    int iframe = 0;
    fprintf(fp,"Start of Frame  ");
    static int nmsgs = 0;

    for (i=0;i<nelems;) {
        int nbytes;
        uint64_t pmap = 0;
        uint64_t field = 0;
        uint64_t scratch = 0;
        fprintf(fp,"Imsg: %d ", nmsgs );
        if (DEBUG_DATA_COUNT) {
            fprintf(fp," PmapSize: %d (0x%X)", buffer[i], buffer[i] );
        }
        fprintf(fp,"\n");

        int buff_offset = 0;
        if (DEBUG_DATA_COUNT) {
            l_encode_field( buffer+i, 7, &scratch, &pmap );
            fprintf(fp,"pmap:   0x%llx \n", pmap );
            fprintf(fp,"decode: 0x%llx \n", scratch );

            fprintf(fp,"\nField Index: %d  ncur_fields:%X  type:%c (0x%x)  tindex:%d\n", buffer[i+sizeof(pmap)],
                    buffer[i+sizeof(pmap)+1],
                    buffer[i+sizeof(pmap)+2],
                    buffer[i+sizeof(pmap)+2],
                    buffer[i+sizeof(pmap)+3]
                   );
            fprintf(fp,"%02X%02X%02X%02X\n", 
                    buffer[i+sizeof(pmap)+4],
                    buffer[i+sizeof(pmap)+5],
                    buffer[i+sizeof(pmap)+6],
                    buffer[i+sizeof(pmap)+7]
                   );
            buff_offset = sizeof(pmap) + sizeof(field);
        }


        uint64_t first_field;
        l_encode_field( buffer+i+buff_offset, 1, &first_field, &field );
        fprintf(fp,"\nFieldSize: %d (0x%X)\n", buffer[i+buff_offset], buffer[i+buff_offset] );
        fprintf(fp,"field encoded:   0x%llx \n", field );
        fprintf(fp,"field decoded: 0x%llx \n\n", first_field );

        int offset = i+buff_offset+sizeof(field);
        for (int iword=0;iword < 8 ;iword++ ){
            unsigned char field_id = 0;
            l_extract_field( buffer+offset+(iword*8), &field, &field_id, &scratch, buffer[offset+8*8+iword] );
            fprintf(fp,"field: %X (scratch %X)  fid: %d  size: %d\n", field, scratch, field_id, buffer[offset+8*8+iword] );
            uint64_t field_val;
            int sz = 0;
            if ( iword == 0 ) {
                field_val = first_field;
                sz =  buffer[i+buff_offset];
            } else {
                field_val = field;
                sz = buffer[offset+8*8+iword];
            }
            if (field_id == 6) {
                char buffer[128];
                char * ptr = buffer;
                l_dump_security_symbol( field_val, &ptr );
                ptr[0] = '\0';
                fprintf(fp,"symbol: %s\n",buffer);
            }
            if (field_id == 25) {
                fprintf(fp,"bid price: %d\n", l_extract_as_int( field_val, sz ));
            }
            if (field_id == 27) {
                fprintf(fp,"offer price: %d\n", l_extract_as_int( field_val, sz ));
            }

            if (field_id == FIRST_MSG_FIELD_INDICATOR) {
                nmsgs ++;
            }
        }


        i+=N_RETURN_BYTES;//COMPRESS_PMAP_SIZE;
    }

}

