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

#include "../host/inc/decode_types.h"
#include "../host/inc/msg_types.h"
#include "../host/inc/aocl_quickudp_channel.h"

// UDP interfaces

channel QUDPWord udp_in_IO __attribute__((depth(0))) __attribute__((io("eth0_in"))); 
channel QUDPWord udp_out_IO __attribute__((depth(0))) __attribute__((io("eth0_out"))); 

channel QUDPWord udp_in __attribute__((depth(0)));
channel QUDPWord udp_out __attribute__((depth(0)));
// Our decoder kernel takes 8 bytes per loop iteration, width adaptor kernels take in 16 bytes from UDP interface 
channel ulong2 input_stream __attribute__((depth(0)));
channel ulong2 output_stream __attribute__((depth(0)));



channel ulong message_stream[FIELDS] __attribute__((depth(63)));

// Utility functions processing UDP channel control info

bool is_eop(ulong ctrl_data) {
   bool ret = (ctrl_data & EOP_MASK) >> EOP_BIT ? true : false;
   return ret;
}

uchar extract_valid_bytes(ulong ctrl_data) {
   uchar a = (ctrl_data & VALID_MASK) >> VALID0_BIT;
   // This field indicates number of EMPTY bytes
   if (is_eop(ctrl_data)) return 8 - a; else return 0;

}

// Utility function counting the number of 1s in a ulong

unsigned char cl_count_ones(unsigned long bits) {
    unsigned long count = bits;
    count = ( count & 0x5555555555555555 ) + ( ( count >> 1 ) & 0x5555555555555555 ) ;
    count = ( count & 0x3333333333333333 ) + ( ( count >> 2 ) & 0x3333333333333333 );
    count = ( count & 0x0f0f0f0f0f0f0f0f ) + ( ( count >> 4 ) & 0x0f0f0f0f0f0f0f0f );
    count = ( count & 0x00ff00ff00ff00ff ) + ( ( count >> 8 ) & 0x00ff00ff00ff00ff );
    count = ( count & 0x0000ffff0000ffff ) + ( ( count >> 16 ) & 0x0000ffff0000ffff );
    count = ( count & 0x00000000ffffffff ) + ( ( count >> 32 ) & 0x00000000ffffffff );
    return (char)count;
}

// Utility function returning a mask matching the nth 1 in an ulong, the count starts from 0
ulong findNthPop(ulong pmap, int n) {
   ulong mask = 1;
   ulong result = 0;
   bool found = false;
   #pragma unroll 64
   for (int i = 0; i < 64; i++) {
      if (!found && cl_count_ones(pmap & mask) == n + 1) {
         result |= 1 << i;
         found = true;
      }
      mask <<= 1;
      mask |= 1;
   }
   return result;
}

// Utility reversing 7 * n bits in an ulong (n is encoded as bit-vector marking
// a bit as 1 for each group of 7 bits)
ulong reverse(ulong x, uchar n) {
   ulong result = 0;
   #pragma unroll 64
   for (int i = 0; i < 64; i++) {
      if ((n >> (i / 7)) & 0x01) {
         result <<= 1; 
         result |= x & 1;
      }
      x >>= 1;
   }
   return result;
}



kernel void io_in_kernel(global ulong4 *mem_read, uchar read_from, int size) {
  int index = 0;
  ulong4 data;
  int half_size = size >> 1;
  while (index < half_size) {
    if (read_from & 0x01) {
      data = read_channel_altera(udp_in_IO);
    } else {
      data = mem_read[index];
    }
    write_channel_altera(udp_in, data);
    index++;
  }
}



kernel void io_out_kernel(global ulong2 *mem_write, uchar write_to, int size) {
  int index = 0;
  ulong4 data;
  int half_size = size >> 1;
  while (index < half_size) {
    ulong4 data = read_channel_altera(udp_out);
    if (write_to & 0x01) {
      write_channel_altera(udp_out_IO, data);
    } else {
      //only write data portion
      ulong2 udp_data;
      udp_data.s0 = data.s0;
      udp_data.s1 = data.s1;
      mem_write[index] = udp_data;
    }
    index++;
  }
}




__attribute__((task))
kernel void width_adapt_2to1() {
  int i = 0;
  int j = 0;
  bool last_eop = 0;
  short last_empty = 0;
  ulong last_y = 0;
  ulong out_data = 0;
  while(1) {
    bool out_sop = false;

    if ( j==0 ) 
    {
      QUDPWord in = read_channel_altera(udp_in);
      ulong2 d = quickudp_get_data(in);
      last_y = d.y;
      out_data = d.x;
      out_sop = quickudp_get_sop(in);
      last_eop = quickudp_get_eop(in);
      last_empty = quickudp_get_empty(in);
    }
    else
    {
      out_data = last_y;
    }

    short out_empty = last_empty < sizeof(ulong) ? last_empty : last_empty - sizeof(ulong);
    bool out_eop = last_eop && ( (j == 0) ? last_empty >= sizeof(ulong) : true);
    bool out_valid =  (j == 0) ? true : (!last_eop || last_empty < sizeof(ulong));
    

    if (out_valid)
    {
      ulong2 out;
      out.y = (out_sop << SOP_BIT) | ( out_eop << EOP_BIT) | (out_empty << VALID0_BIT);
      out.x = out_data;
      write_channel_altera(input_stream, out);
    }

    j = (j == 0) ? 1 : 0;
  }
}

__attribute__((task))
kernel void width_adapt_1to2() {
  bool last_valid = false;
  bool last_sop = false;
  ulong last_data = 0;
  bool out_valid = false;

  while(1) {
    QUDPWord out = (ulong4)(0,0,0,0);
    ulong2 in = read_channel_altera(output_stream);
    bool in_sop = in.y & SOP_MASK;
    bool in_eop = in.y & EOP_MASK;
    short in_empty = (in.y & VALID_MASK) >> VALID0_BIT;

    short out_empty = in_empty;
    if (last_valid)
    {
      ulong2 odata = (ulong2)(last_data, in.x);
      quickudp_set_data(&out,odata);
      quickudp_set_sop(&out,last_sop);
      out_valid = true;
    }
    else 
    {
      ulong2 odata = (ulong2)(in.x, 0);
      quickudp_set_data(&out,odata);
      out_empty |= sizeof(ulong);
      out_valid = in_eop;
      quickudp_set_sop(&out,in_sop);
    }

    quickudp_set_eop(&out, in_eop, out_empty);
    if ( in_eop )
      out_valid = true;

    if ( out_valid )
    {
      write_channel_altera(udp_out, out);
      last_valid = false;
    } else 
      last_valid = true;

    last_sop = in_sop;
    last_data = in.x;
  }
}


// OPRA decoder kernel

__attribute__((task))
kernel void OPRADecoder(int size) {
   int index = 0;
   uchar left = OPRA_HEADER_SIZE; // keeps track of bytes left in current message
   bool valid = false; // current message is the OPRA header, so not valid

   ulong crt_field = 0; // carries a partially parsed field
   uchar crt_length = 0; // bit-vector marking the number of bytes received for crt_field

   ulong pmap = 0; // carries current message's pmap
   uchar pmapLength; // carries the length of the pmap
   bool pmapParsed = false; // flags whether the pmap of current message was parsed

   uchar fieldCount = 0; // counts fields parsed in the current message

   uchar8 stream_data; // UDP data - 8 bytes / word
   ulong ctrl_data;  // ctrl word associated with data word

   ulong msg[FIELDS]; // message data

   // During each window handles up to two messages:
   //    (a) the end of an "old" message that may have started in a previous window
   //    (b) a "new" message that starts in this window
   // If the "new" message completes in this window, stall on this frame
   // This is a rare case and should not affect performance

   bool persist = false; // flags whether the new iteration should reprocess previous word

   while (index < size) {
      // read a word, unless the previous word was not fully used
      if (!persist) {
         ulong2 data;
         data = read_channel_altera(input_stream);
      
         index++;
         ctrl_data = data.y;
         ulong long_data = data.x;
         stream_data = *((uchar8 *)&(long_data)); // cast to uchar8 for easy access to elements
      }

      // extract information from the control packet
      uchar valid_bytes = extract_valid_bytes(ctrl_data);
      bool eop = is_eop(ctrl_data);

      // Flag whether a new message starts in this word, or whether an old
      // message finished
      
      bool startsNew = left < ETHERNET_WORD_SIZE;
      bool finishedOld = left > 0 && left <= ETHERNET_WORD_SIZE;

      int newLen = stream_data[left];

      // No message should have length 0, this can only happen if reaching
      // the CRC (which we don't track); In that case, that is the only byte
      // in the message and we would reset the tracking as soon as we get an
      // EOP; to prevent triggering a new message if we read a length of 0,
      // increase it to 1

      if (newLen == 0) newLen = 1;

      // The current word will persist if a new message, that starts in this,
      // window, also finishes in this window
      //
      // The condition is to finish the current message in this window, as well
      // as the next message (whose length I just read). It is also important
      // that the next message exists
      
      bool persist = startsNew && (newLen + left < valid_bytes) 
                               && (left < valid_bytes);

      // Create a mask to ignore message length between the old and the new
      // message

      uchar ignore = 0x00; 

      // For parsing purposes, mask the length information of the subsequent
      // mssage

      if (startsNew) {
         ignore = 1 << left;
      }

      // Look at the length of the next message (if any) and update the number
      // of bytes left

      if (startsNew) {
         left += newLen + 1;
      }

      // If this message does not persist, the left length is decreased by the
      // number of bytes processed in this message; otherwise, the length left
      // indicates where the first unprocessed message starts

      if (!persist) {
         left -= ETHERNET_WORD_SIZE;
      }

      // If this is the last packet, ignore the remaining bytes in the window
      // and offset left to skip the header of the OPRA packet

      if (eop && !persist) {
         left = OPRA_HEADER_SIZE;
         ignore |= (0xFF << valid_bytes);
      }
      
      // Parse fields from frame in parallel, there can be up to 
      // ETHERNET_WORD_SIZE fields

      ulong fields[ETHERNET_WORD_SIZE]; 
      
      // Tracks the number of characters in the parsed field

      uchar length[ETHERNET_WORD_SIZE]; 

      // Keep track of number of fields extracted

      uchar extractedFieldCount = 0;
      uchar oldFieldCount = 0;
      bool new = false; // keeps track of which message is parsed

      #pragma unroll ETHERNET_WORD_SIZE
      for (int i = 0; i < ETHERNET_WORD_SIZE; i++) {
         fields[i] = 0;
         length[i] = 0; 
      }

      // Parse word in parallel

      #pragma unroll ETHERNET_WORD_SIZE
      for (int i = 0; i < ETHERNET_WORD_SIZE; i++) {

         // Skip processing for ignored bytes

         if ((ignore >> i) & 0x01) { 
            crt_field = 0;
            crt_length = 0;
            new = true;
            continue;
         }
         
         // Actual parsing - rotate current field 7 bits
         uchar crt_byte = stream_data[i] & 0x7F;
         crt_field = (crt_field << 7) | crt_byte;
         crt_length = (crt_length << 1) | 1;

         // Latch result if this is the last byte of a field

         if (stream_data[i] & 0x80) {
            // Store parsed field
            length[extractedFieldCount] = crt_length;
            fields[extractedFieldCount++] = crt_field;
            if (!new) oldFieldCount = extractedFieldCount;
            crt_field = 0;
            crt_length = 0;
         }
      }

      // Presence map (Pmap) availability

      // "new" message Pmap is available if at least one field of the new
      // message has been parsed

      bool haveNewPmap = (startsNew && extractedFieldCount > oldFieldCount);

      // "old" Pmap is available if it was parsed in a previous word or at 
      // least one field has been parsed  

      bool haveOldPmap = pmapParsed || extractedFieldCount > 0;
   
      // Flags if "old" Pmap has been parsed in this word

      bool oldPmapJustParsed = (!pmapParsed) && extractedFieldCount > 0;

      // Based on logic above, extract pmap from parsed fields, if required

      ulong oldPmap = pmapParsed ? pmap : fields[0];
      ulong newPmap = haveNewPmap ? fields[oldFieldCount] : 0;

      uchar oldPmapLength = pmapParsed ? pmapLength : length[0];
      uchar newPmapLength = haveNewPmap ? length[oldFieldCount] : 0;

      // Carries the Pmap parsing status to the new word

      pmapParsed = startsNew ? haveNewPmap : haveOldPmap;
      
      // Carries the parsed Pmap to the subsequent iteration
      // If the frame persists, overwrite pmap as 0 to prevent recording
      // fields from messages that have been already parsed
      
      pmap = persist ? 0 : (startsNew ? newPmap : oldPmap);
      pmapLength = persist ? 0 : (startsNew ? newPmapLength : oldPmapLength);

      // Reverse the pmap bits in the representation for the actual
      // interpretation

      oldPmap = reverse(oldPmap, oldPmapLength);
      newPmap = reverse(newPmap, newPmapLength);

      // Based on oldPmap and newPmap and the info about the fields parsed, 
      // compute field assignments

      // First, compute masks on Pmap for each parsed field
      // Such a mask indicates in bit-vector form where to store the parsed
      // field in the message

      ulong oldPmapMasks[ETHERNET_WORD_SIZE], newPmapMasks[ETHERNET_WORD_SIZE];

      #pragma unroll ETHERNET_WORD_SIZE
      for (int i = 0; i < ETHERNET_WORD_SIZE; i++) {
         
         // First "oldFieldCount" fields belong to the "old" message
         // May exclude field 0 if it was the pmap
         // Identify on Pmap vector which is the corresponding pop
         // The index takes into account that "fieldCount" fields in the same
         // message may have been processed in previous words, unless the pmap
         // has just been processed, case when this is the beginning of the
         // message
         
         oldPmapMasks[i] = newPmapMasks[i] = 0;
         bool isOld = (i < oldFieldCount) && (i > 0 || !oldPmapJustParsed);
         if (isOld) {
            oldPmapMasks[i] = findNthPop(oldPmap, oldPmapJustParsed ? i - 1 : fieldCount + i);
            newPmapMasks[i] = oldPmapMasks[i];
         }

         // Subsequent fields belong to the "new" message; skip the new "Pmap"
         // Identify on Pmap vector which is the corresponding pop
         
         bool isNew = i > oldFieldCount && i < extractedFieldCount;
         if (isNew) {
            newPmapMasks[i] = findNthPop(newPmap, i - oldFieldCount - 1);
         }
      }

      // For each message field, create a bit vector indicating which parsed 
      // field is its source. The message field can be updated with one of the 
      // parsed fields, or maintain its previous value (no bit set in the
      // vector)

      uchar oldSourceMasks[FIELDS];
      uchar newSourceMasks[FIELDS];

      #pragma unroll FIELDS
      for (int i = 0; i < FIELDS; i++) {
         uchar oldMask = 0, newMask = 0;
         #pragma unroll ETHERNET_WORD_SIZE
         for (int j = 0; j < ETHERNET_WORD_SIZE; j++) {

            // Inspect masks computed above for information corresponding to
            // message fields

            oldMask |= oldPmapMasks[j] & (1L << i) ? (1 << j) : 0;
            newMask |= newPmapMasks[j] & (1L << i) ? (1 << j) : 0;
         }

         // Store computed bit-vectors

         oldSourceMasks[i] = oldMask;
         newSourceMasks[i] = newMask;
      }

      // Finally, update the messages
      // The final update may include a partially processed new message. Use
      // "completedMessage" to store only the changes from the "old" message

      ulong completedMessage[FIELDS];

      // Finally, update the fields
      #pragma unroll FIELDS
      for (int i = 0; i < FIELDS; i++) {

         // Get the previous value of each field

         ulong oldField = msg[i];
         ulong newField = msg[i];

         // Adjust values with FAST_OP_INCR
         
         if (i == MESSAGE_SEQUENCE_NUMBER_V2 && startsNew) newField++;

         #pragma unroll ETHERNET_WORD_SIZE
         for (int j = 0; j < ETHERNET_WORD_SIZE; j++) {

            // Loads the bit vector indicating whether there is any update
            // There may be updates from both the "old" and the "new" message
            // In that case, "newMatch" will pick the latter

            bool oldMatch = oldSourceMasks[i] & (1 << j);
            bool newMatch = newSourceMasks[i] & (1 << j);
            oldField = oldMatch ? fields[j] : oldField;
            newField = newMatch ? fields[j] : newField;
         }
         // Update the message carried on as well as the completed message
         msg[i] = newField;
         completedMessage[i] = oldField;
      }

      // Update the fieldCount based on the fields parsed in this word

      if (startsNew) {
         if (haveNewPmap) {
            fieldCount = extractedFieldCount - oldFieldCount - 1;
         } else {
            fieldCount = 0;
         }
      } else {
         fieldCount = oldPmapJustParsed ? oldFieldCount - 1 : fieldCount + oldFieldCount;
      }

      // Passes completed message for subsequent processing
      #define WRITE(i) write_channel_altera(message_stream[i], completedMessage[i]);

      if (finishedOld && valid) {
         WRITE(0); WRITE(1); WRITE(2); WRITE(3); WRITE(4); 
         WRITE(5); WRITE(6); WRITE(7); WRITE(8); WRITE(9); 
         WRITE(10); WRITE(11); WRITE(12); WRITE(13); WRITE(14); 
         WRITE(15); WRITE(16); WRITE(17); WRITE(18); WRITE(19); 
         WRITE(20); WRITE(21); WRITE(22); WRITE(23); WRITE(24); 
         WRITE(25); WRITE(26); WRITE(27); WRITE(28); WRITE(29); 
         WRITE(30); WRITE(31); WRITE(32); WRITE(33); WRITE(34); 
         WRITE(35); WRITE(36); WRITE(37); WRITE(38); WRITE(39); 
         WRITE(40); WRITE(41); WRITE(42); WRITE(43); WRITE(44); 
         WRITE(45); WRITE(46); WRITE(47); WRITE(48); WRITE(49); 
      }

      // Mark next message as invalid if it is the OPRA header

      if (eop && !persist) {
         valid = false;
      } else {
        if (finishedOld) valid = true;
      }
   }
}

// Dummy trading algorithm
// Simply return a subset of the fields in the message to the host

__attribute__((task))
kernel void Trading( uchar start_field, uchar count, int udp_packet_length, int size) {
   int index = 0;
   uchar which_field = start_field;
   ushort udp_length = 0;
   while (index < size) {

      ulong write_what = 0, tmp = 0;

      #define READ(i) \
         if ((which_field == i) ||  \
             (which_field == start_field && ((i < start_field) || (i >= start_field + count)))) { \
            tmp = read_channel_altera(message_stream[i]); \
         } \
         if (which_field == i)  write_what = tmp;

      READ(0); READ(1); READ(2); READ(3); READ(4); 
      READ(5); READ(6); READ(7); READ(8); READ(9); 
      READ(10); READ(11); READ(12); READ(13); READ(14); 
      READ(15); READ(16); READ(17); READ(18); READ(19); 
      READ(20); READ(21); READ(22); READ(23); READ(24); 
      READ(25); READ(26); READ(27); READ(28); READ(29); 
      READ(30); READ(31); READ(32); READ(33); READ(34); 
      READ(35); READ(36); READ(37); READ(38); READ(39); 
      READ(40); READ(41); READ(42); READ(43); READ(44); 
      READ(45); READ(46); READ(47); READ(48); READ(49); 
      
      ulong ctrl = 0;
      if (udp_length == 0) ctrl |= SOP_MASK;
      if ((udp_length == udp_packet_length - 1) || (index == size - 1)) {
         ctrl |= EOP_MASK;
         udp_length = 0;
      } else {
         udp_length++;
      }

      ulong2 data = (ulong2)(write_what, ctrl);
     
      write_channel_altera(output_stream, data);

      index++;
      if (which_field < start_field + count - 1) which_field++; else which_field = start_field;
   }
   
}
