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

// File parsing functions - parses
// pcap files or OPRA demo format files
//
// Copyright 2012 Altera Corporation.

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "file_parse.h"

// This file contains helper functions for reading OPRA files

// file parsing fields
static unsigned char l_cur_pkt_buffer[MAX_FRAME_SIZE];
FILE*         l_cur_pcap_file   = NULL;
static int           l_cur_index       = -1;
static int           l_cur_frame_sz    = -1;
static const char*         l_cur_filename    = NULL;
static const char*         l_cur_file_format = NULL;

// file profiling fields
static int l_min_msg = 9999;
static int l_max_msg = 0;
static int l_max_nmsgs_per_frame = 0;
static int l_iframe = 0;
static int l_use_pcap = 1;

static void 
l_check_pkt (unsigned char*pkt_buffer, int buf_len, FILE*dump) 
{
    int cur_byte = FRAME_HEADER_LEN;
    l_iframe ++;
    int nmsgs = ( pkt_buffer[cur_byte-1] - '0') +
        10* ( pkt_buffer[cur_byte-2] - '0') +
        100* ( pkt_buffer[cur_byte-3] - '0');
    l_max_nmsgs_per_frame = nmsgs > l_max_nmsgs_per_frame ? nmsgs : l_max_nmsgs_per_frame;
    if (dump) {
        fprintf(dump,"Frame %d: ", l_iframe );
        for (int i=0;i<FRAME_HEADER_LEN;i++) {
            fprintf(dump,"%02X ",pkt_buffer[i]);
        }
        fprintf(dump," - ");
    }
    while (cur_byte < buf_len ) {
        int msize = pkt_buffer[cur_byte];

        l_min_msg = l_min_msg > msize ? msize : l_min_msg;
        l_max_msg = l_max_msg < msize ? msize : l_max_msg;

        if (msize < 8) {
            printf("iframe: %d\n",l_iframe);
        }
        if (dump) {
            for (int i=0;i<msize+1;i++) {
                fprintf(dump,"%02X ",pkt_buffer[cur_byte+i]);
            }
            fprintf(dump,"\n");
        }
        cur_byte += msize+1;
        if (cur_byte == buf_len-1) {
            assert( pkt_buffer[cur_byte] == ETX );
            break;
        }
    }
}

void print_file_stats() {
    printf("max msg size: %d\n", l_max_msg );
    printf("min msg size: %d\n", l_min_msg );
    printf("max nmsgs: %d\n", l_max_nmsgs_per_frame );
}

int 
get_pkt( FILE* fp, unsigned char* pkt_buffer, int buf_len)
{
    int frame_len = 0;
    int len;

    if (l_use_pcap) {
        len = fread( pkt_buffer, 1, PCAP_FRAME_HEADER_SIZE, fp );
        assert(len == PCAP_FRAME_HEADER_SIZE || len == 0 );
        if (!len) {
            return 0;
        }

        len = fread( pkt_buffer, 1, ETHERNET_HEADER_SIZE+IP4_HEADER_SIZE, fp );
        assert(len == ETHERNET_HEADER_SIZE+IP4_HEADER_SIZE );

        len = fread( pkt_buffer, 1, UDP_HEADER_SIZE, fp );
        assert(len == UDP_HEADER_SIZE );
        frame_len = ( ((unsigned int)pkt_buffer[UDP_SIZE_OFFSET]) << 8 ) + (unsigned int)pkt_buffer[UDP_SIZE_OFFSET+1] - UDP_HEADER_SIZE;
    } else {
        unsigned char buf[sizeof(unsigned int)];
        len = fread(buf, sizeof(unsigned int),  1, fp);
        // len = fread(&frame_len, sizeof(unsigned int),  1, fp);
        assert(len == 1 || len == 0);
        if (len == 0) {
            return 0;
        }
        frame_len = buf[3] | buf[2] << 8 | buf[1] << 16 | buf[0] << 24;
#ifdef LINUX
#ifdef USE_NTOHL
        frame_len = ntohl(frame_len);
#endif
#endif
    }

    assert( buf_len >= frame_len );
    memset( pkt_buffer, '\0', buf_len );
    len = fread( pkt_buffer, 1, frame_len, fp );
    assert(len == frame_len);
    return frame_len;
}

void init_input_read( const char * filename, const char* file_format )
{
    if ( l_cur_pcap_file != NULL ) {
        fclose( l_cur_pcap_file );
    }

    l_cur_pcap_file = fopen( filename, "rb" );
    assert(l_cur_pcap_file);

    l_use_pcap = !strcmp( file_format, "pcap" );
    l_cur_file_format = file_format;
    if (l_use_pcap) {
        int elems_read = fread( l_cur_pkt_buffer, PCAP_FILE_HEADER_SIZE, 1, l_cur_pcap_file );
    }
    l_cur_index = -1;
    l_cur_frame_sz = -1;
    l_cur_filename = filename;
}

void fill_pkt_data( int64_t* data, int * frames, int nelems, FILE*dump, int add_ctrl )
{
    int ielem = 0;
    int frame = 0;
    int ibyte = 0;
    int stride = 1;

    if (add_ctrl) {
        stride = 2;
    }

    assert(l_cur_pcap_file);
    while (ielem < nelems) {
        if ( l_cur_index == l_cur_frame_sz || l_cur_frame_sz == -1 ) {
#ifdef APPLY_SKIP
            // code to skip to frame 4
            static int skip = 1;
            // fill frame buffer
            if ( skip ) {
                skip = 0;
                get_pkt( l_cur_pcap_file, l_cur_pkt_buffer, MAX_FRAME_SIZE );
                get_pkt( l_cur_pcap_file, l_cur_pkt_buffer, MAX_FRAME_SIZE );
                get_pkt( l_cur_pcap_file, l_cur_pkt_buffer, MAX_FRAME_SIZE );
                get_pkt( l_cur_pcap_file, l_cur_pkt_buffer, MAX_FRAME_SIZE );
            }
#endif
            while ( ( l_cur_frame_sz = get_pkt( l_cur_pcap_file, l_cur_pkt_buffer, MAX_FRAME_SIZE ) ) == 0 ) {
                init_input_read( l_cur_filename, l_cur_file_format );
            }
            l_check_pkt( l_cur_pkt_buffer, l_cur_frame_sz, dump );
            l_cur_index = 0;
        }
        if (ibyte == 0) {
            data[stride*ielem] = 0;
            if (add_ctrl) {
                data[stride*ielem+1] = 0;
            }
        }
        data[stride*ielem] |= ( (uint64_t)(0xff&l_cur_pkt_buffer[l_cur_index]))  << (8*ibyte);
        if ( l_cur_index == ( l_cur_frame_sz - 1 )) {
             frames[frame++] = l_cur_frame_sz;
        }
        if (add_ctrl) {
            if (l_cur_index == 0) {
                assert(ibyte == 0);
                data[stride*ielem+1] |= SOP_MASK;
            }

            if ( l_cur_index == ( l_cur_frame_sz - 1 )) {
                data[stride * ielem+1] |= EOP_MASK;
                data[stride * ielem + 1] |= (l_cur_index % 8) << VALID0_BIT;
            }
        }
        ibyte++;
        l_cur_index++;
        if (ibyte == sizeof(data[0]) || (l_cur_index == l_cur_frame_sz) ) {
            ibyte = 0;
            ielem++;
        }
    }
    if (add_ctrl) {
       data[stride * ielem - 1] |= EOP_MASK;
       data[stride * ielem - 1] |= ((l_cur_index - 1) % 8) << VALID0_BIT;
    }
    frames[frame] = l_cur_index;
}

FILE* get_cur_fp() 
{
    return l_cur_pcap_file;
}
