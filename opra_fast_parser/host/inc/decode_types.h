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

//
// Define OPRA FAST decode fields
//

#ifndef __DECODE_TYPES_H__
#define __DECODE_TYPES_H__

#include "msg_types.h"


typedef enum fast_op_t
{
   FAST_OP_NONE = 0,
   FAST_OP_COPY,
   FAST_OP_INCR,
   FAST_OP_DELTA,
}
fast_op_t;

typedef enum fast_type_t
{
   FAST_TYPE_NULL = 0,
   FAST_TYPE_U32,
   FAST_TYPE_I32,
   FAST_TYPE_STR,
   FAST_TYPE_U64,
}
fast_type_t;

#define VERSION_2 (2)
#define MAX_TAG 64

#define TAG_MAX_SLOT     0xff // extendable to 0xfff
#define TAG_MAX_TID      0xf  // extendable to 0xfff
#define TAG_MAX_OP       0xf
#define TAG_MAX_TYPE     0xf

#define TAG_SHIFT_SLOT    0
#define TAG_SHIFT_TID    12
#define TAG_SHIFT_OP     24
#define TAG_SHIFT_TYPE   28

#define MAKE_TAG(type,op,tid,slot)  (slot)

enum opra_tid
{
    OPRA_BASE_TID=0
};

enum opra_msg_field_tags_v2
{
        // opra header

        MESSAGE_CATEGORY_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 0),
        MESSAGE_TYPE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 1),
        PARTICIPANT_ID_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 2),
        RETRANSMISSION_REQUESTER_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 3),
        MESSAGE_SEQUENCE_NUMBER_V2 =
            MAKE_TAG (FAST_TYPE_U64, FAST_OP_INCR, OPRA_BASE_TID, 4),
        TIME_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 5),

    	//sequential order 

        SECURITY_SYMBOL_V2 = 
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 6),
        EXPIRATION_MONTH_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 7),
        EXPIRATION_DATE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 8),
        YEAR_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 9),
        STRIKE_PRICE_DENOMINATOR_CODE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 10),
        EXPLICIT_STRIKE_PRICE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 11),
        STRIKE_PRICE_CODE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 12),
        VOLUME_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 13),
        OPEN_INT_VOLUME_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 14),
        PREMIUM_PRICE_DENOMINATOR_CODE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 15),
        PREMIUM_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 16),
        OPEN_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 17),
        HIGH_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 18),
        LOW_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 19),
        LAST_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 20),
        NET_CHANGE_INDICATOR_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 21),
        NET_CHANGE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 22),
        UNDERLYING_PRICE_DENOM_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 23),
        UNDERLYING_STOCK_PRICE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 24),

        BID_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 25),
        BID_SIZE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 26),
        OFFER_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 27),
        OFFER_SIZE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 28),
        SESSION_INDICATOR_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 29),
        BBO_INDICATOR_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 30),

        BEST_BID_PARTICIPANT_ID_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 31),
        BEST_BID_PRICE_DENOMINATOR_CODE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 32),
        BEST_BID_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 33),
        BEST_BID_SIZE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 34),

        BEST_OFFER_PARTICIPANT_ID_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 35),

        BEST_OFFER_PRICE_DENOMINATOR_CODE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 36),
        BEST_OFFER_PRICE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 37),
        BEST_OFFER_SIZE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 38),
    	NUMBER_OF_INDICES_IN_GROUP_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 39),
    	NUMBER_OF_FOREIGN_CURRENCY_SPOT_VALUES_IN_GROUP_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 40),
    	INDEX_SYMBOL_V2 = 
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 41),
    	INDEX_VALUE_V2 =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 42),
    	BID_INDEX_VALUE_V2 =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 43),
    	OFFER_INDEX_VALUE_V2 =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 44),
    	FCO_SYMBOL_V2 =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 45),
    	DECIMAL_PLACEMENT_INDICATOR_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 46),
    	FOREIGN_CURRENCY_SPOT_VALUE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 47),
    	TEXT_V2 =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 48),
    	DEF_MSG_V2 =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 49),
};
  
#define SECURITY_SYMBOL_SIZE 5

#define FIRST_MSG_FIELD_INDICATOR 0x80
#define INVALID_FIELD_INDICATOR   0xff

#endif // __MSG_TYPES_H__
