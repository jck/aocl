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

#ifndef __MSG_TYPES_H__
#define __MSG_TYPES_H__
 
#ifdef ALTERA_CL
typedef long int64_t;
typedef ulong uint64_t;
#else
#include <inttypes.h>
#endif
 
////////////////////////////////////////////
// Define this value if you simply want to
// read data into a kernel,
//
// Otherwise, data will be read back to host, adding
// latency
//
// #define DECODE_READER


////////////////////////////////////////////
// Define this if you want to send
// 2 words of debug data, along with
// decoded field data
// #define SEND_EXTRA_DEBUG_DATA


#define DUMP_RAW_DATA


#define SOP_BIT  0
#define EOP_BIT  1
#define VALID0_BIT 2
#define SOP_MASK (0x1 << SOP_BIT)
#define EOP_MASK (0x1 << EOP_BIT)
#define CTRL_SOP(ctrl_data) (((ctrl_data) & SOP_MASK) >> (SOP_BIT))
#define CTRL_EOP(ctrl_data) (((ctrl_data) & EOP_MASK) >> (EOP_BIT))
#define VALID_MASK (0x0f << VALID0_BIT)


// If we're adding the CTRL word (EOP/SOP), we'll send 2 words per
// word of data
#define USE_CTRL 1
#if USE_CTRL
#define WORDS_PER_ELEM 2
#else
#define WORDS_PER_ELEM 1
#endif

#ifdef SEND_EXTRA_DEBUG_DATA
#define DEBUG_DATA_COUNT 2
#else
#define DEBUG_DATA_COUNT 0
#endif

#define N_RETURN_BYTES ((DEBUG_DATA_COUNT+1+9)*8)

#define PCAP_FILE_HEADER_SIZE  ( 24 )
#define PCAP_FRAME_HEADER_SIZE ( 16 )
#define ETHERNET_HEADER_SIZE   ( 14 )
#define IP4_HEADER_SIZE        ( 20 )
#define UDP_HEADER_SIZE        ( 8 )
#define UDP_SIZE_OFFSET        ( 4 )

// offsets for V2 messages
#define SEQ_OFFSET 		10
#define VERSION_OFFSET 	1
#define NUM_MSG_OFFSET 	3
#define SOH_OFFSET      1
#define FRAME_HEADER_LEN ( SEQ_OFFSET + VERSION_OFFSET + NUM_MSG_OFFSET + SOH_OFFSET )

#define US           0x1F       // Unit Separator
#define SOH          0x01
#define VER          0x02
#define ETX          0x03       // End of Text

#define MAX_FRAME_SIZE 1456

#define NMSG_START_BYTE  ( 12 )
#define NMSG_SIZE        ( 3 )
#define FIELDS 50
#define OPRA_HEADER_SIZE 15


#define MSG_HEADER_SIZE  ( 1 )
#define ETX_SIZE         ( 1 )

#define ETHERNET_WORD_SIZE   8
#endif
