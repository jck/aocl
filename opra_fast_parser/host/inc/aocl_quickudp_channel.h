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


#define QUDP_SOP_BIT  0
#define QUDP_EOP_BIT  1
#define QUDP_EMPTY0_BIT 2
#define QUDP_EMPTY1_BIT 3
#define QUDP_EMPTY2_BIT 4
#define QUDP_EMPTY3_BIT 5
#define QUDP_SIZE0_BIT 6
#define QUDP_ERROR_BIT 14
#define QUDP_SOP_MASK (0x1L << QUDP_SOP_BIT)
#define QUDP_EOP_MASK (0x1L << QUDP_EOP_BIT)
#define QUDP_CTRL_SOP(ctrl_data) (((ctrl_data) & QUDP_SOP_MASK) >> (QUDP_SOP_BIT))
#define QUDP_CTRL_EOP(ctrl_data) (((ctrl_data) & QUDP_EOP_MASK) >> (QUDP_EOP_BIT))
#define QUDP_EMPTY_MASK (0x0fL << QUDP_EMPTY0_BIT)
#define QUDP_SIZE_MASK (0x0ffffL << QUDP_SIZE0_BIT)
#define QUDP_ERROR_MASK (0x1L << QUDP_ERROR_BIT)

typedef ulong4 QUDPWord;

bool quickudp_get_sop (QUDPWord beat);
void quickudp_set_sop (QUDPWord *beat, bool sop);
bool quickudp_get_eop (QUDPWord beat);
void quickudp_set_eop (QUDPWord *beat, bool eop, short empty_bytes);
short quickudp_get_empty (QUDPWord beat);
ulong2 quickudp_get_data (QUDPWord beat);
void quickudp_set_data (QUDPWord *beat, ulong2 data);

/***************************************************/
/****************** Implementation *****************/
/***************************************************/

bool quickudp_get_sop (QUDPWord beat) {
  return (beat.z & QUDP_SOP_MASK) != 0;
}

void quickudp_set_sop (QUDPWord *beat, bool sop) {
  if ( sop )
    (*beat).z |= (ulong)(1L << QUDP_SOP_BIT);
  else
    (*beat).z &= ~(ulong)(1L << QUDP_SOP_BIT);
  // Set size to 0 to let QUDP infer size from sop/eop
  ulong size_mask = QUDP_SIZE_MASK;
  (*beat).z &= ~size_mask;
}

bool quickudp_get_eop (QUDPWord beat) {
  return (beat.z & QUDP_EOP_MASK) != 0;
}

void quickudp_set_eop (QUDPWord *beat, bool eop, short empty_bytes) {
  if ( eop )
  {
    (*beat).z |= (ulong)(1L << QUDP_EOP_BIT);
    (*beat).z = ((*beat).z & ~((ulong)QUDP_EMPTY_MASK)) | (((ulong)empty_bytes) << QUDP_EMPTY0_BIT);
  }
  else
    (*beat).z &= ~(ulong)(1L << QUDP_EOP_BIT);
}

short quickudp_get_empty (QUDPWord beat) {
  return (beat.z & QUDP_EMPTY_MASK) >> QUDP_EMPTY0_BIT;
}

ulong2 quickudp_get_data (QUDPWord beat) {
  ulong2 r;
  r.x = beat.x;
  r.y = beat.y;
  return r;
}

void quickudp_set_data (QUDPWord *beat, ulong2 data) {
  (*beat).x = data.x;
  (*beat).y = data.y;
}
