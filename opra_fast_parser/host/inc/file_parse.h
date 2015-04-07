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

#ifndef __FILE_PARSE_H__
#define __FILE_PARSE_H__

#include <stdio.h>

#include "msg_types.h"

///////////////////////////////////////////////////////////////
// start the file reader
//
// Call this function before calling other file parsing functions
// 
// filename - name of the file as a string
// file_format - format of the file, valid formats are "pcap" and
//               "opra"
void init_input_read( const char * filename, const char*file_format );

///////////////////////////////////////////////////////////////
// read the file. This function will read the file stream initialized
// by init_input_read, and store the pkt data into the data buffer.
// If the file size is smaller than the data buffer, it will continually
// read the file and concat the data togethre until the data buffer is
// full.
//
// If you call fill_pkt_data multiple times, the file reading will start
// from the previous file location is ended off in the last
// fill_pkt_data call.
//
// Must call init_input_read before calling this function
//
// data      - buffer to store packet data
// nelems    - buffer size
// dump      - file to dump package data into, can set to NULL
//             if dumping is not needed
// add_ctrl  - add eop/sop signals
void fill_pkt_data( int64_t* data,  int *frames, int nelems, FILE*dump, int add_ctrl );

///////////////////////////////////////////////////////////////
// This gets a single package from the file and
// stores it into the buffer. The size of the packet in
// bytes is returned. If you are at the
// end of the file, pkt_buffer will contain undefined
// data and the return size will be 0.
//
// fp - file pointer to file stream
// pkt_buffer - buffer to store packet data
// buf_len - size of pkt_buffer in bytes
int get_pkt( FILE* fp, unsigned char* pkt_buffer, int buf_len);

///////////////////////////////////////////////////////////////
// Returns the current file handle that is being processed
FILE* get_cur_fp();

///////////////////////////////////////////////////////////////
// This dumps out various packet file statistics specific to
// OPRA including
//
// the maximum message size, 
// the minimum message size,
// the maximum number of messages per packet,
void print_file_stats();

#endif
