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

#ifndef MSG_IO_H
#define MSG_IO_H

#include <stdio.h>

#include "msg_types.h"

typedef enum {
    FILE_DUMP    = 0,
    SCREEN_DUMP  = 1,
    GUI_DUMP     = 2
} dump_t;

void dump_frame   ( char*fname, unsigned int * buffer, int nelems ) ;
void dump_raw_input  ( FILE*fp, int64_t * buffer, int nelems ) ;
void dump_output  ( char*fname, unsigned int * buffer, int nelems ) ;


void pack_char_into_intbuf( unsigned char* buffer, int nelems, unsigned int * intbuf, int n_int_elems ) ;
int create_byte_msg   ( unsigned int msg_size, unsigned char* buffer, int nelems );
int create_byte_frame ( unsigned char* buffer, int nelems );

void init_screen_dump();
void dump_byte_raw_input  ( char*fname, unsigned char * buffer, int nelems ) ;
void dump_byte_output ( char*fname, unsigned char * buffer, int nelems ) ;
void dump_raw_words( FILE*fp, unsigned char * buffer, int nelems ) ;
void dump_decoded_word( FILE*fp, unsigned char * buffer, int nelems ) ;
void dump_decoded_msg( FILE*fp, unsigned long* buffer, int nelems, dump_t dump_type );



#endif
