/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_FSK4_RDLAP_F_IMPL_H
#define INCLUDED_FSK4_RDLAP_F_IMPL_H

#include <fsk4/rdlap_f.h>

// rdlap frame sync buffer length:  should be 48 symbols
// first 24 will be sync sequence; next 24 will be station ID slot
#define NFRAMER_BUFFER 48


// Note that data has been offset, symbols are in {0,1,2,3} set 
// (having been remapped from -3, -1, +1, +3)
#define LEN_RDLAP_SYNC 24
static const unsigned char rdlap_sync[LEN_RDLAP_SYNC] = {
	1,2,1,2,1,3,0,3,0,1,2,0,3,3,1,2,0,0,2,3,1,0,2,3
};


// RD-LAP block interleave for symbol pairs; entry i gives offset into received
// block of i'th symbol pair.  There are 69 symbols in a block (index 0-68) & we
// skip 3 status symbols at positions 22, 45, and 68 leaveing 33 symbol pairs as
// an output that can be sent through a Viterbi algorithm
static const int block_interleave[64]={
  0, 18, 35, 52, 
  2, 20, 37, 54,
  4, 23, 39, 56, 
  6, 25, 41, 58,
  8, 27, 43, 60,
 10, 29, 46, 62,
 12, 31, 48, 64,
 14, 33, 50, 66,
 16
};

namespace gr {
  namespace fsk4 {

    class rdlap_f_impl : public rdlap_f
    {
     private:
        gr::msg_queue::sptr d_queue;
        
        // ----- rd-lap processing: framer detects & splits off frame sync
        void framer(unsigned char sym);
        int framer_state;
        unsigned char framer_buffer[NFRAMER_BUFFER];
        int iframer_buffer;

        // ----- all higher level processing takes place in groups of 69 symbols...
        void accumulate_block(unsigned char sym);
        int accumulate_block_index;
        unsigned char accumulate_block_buffer[128];

        int processed_blocks_since_sync;

        void process_block();

        // ----- viterbi error correction related
        int  viterbi_SED(int s1, int s2);
        void viterbi_reset(void);
        void viterbi_add_data(int cc);
        int  viterbi_return_data(unsigned char *data);


        // ----- header block crc
        int test_CRC2(unsigned char *dd);


        bool reverse_polarity;


     public:
      rdlap_f_impl(gr::msg_queue::sptr queue, int processing_flags);
      ~rdlap_f_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace fsk4
} // namespace gr

#endif /* INCLUDED_FSK4_RDLAP_F_IMPL_H */

