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

#ifndef INCLUDED_FSK4_APCO25_F_IMPL_H
#define INCLUDED_FSK4_APCO25_F_IMPL_H

#include <fsk4/apco25_f.h>

#define NFRAMER_BUFFER_APCO25 24


#define LEN_APCO25_SYNC 24
static const unsigned char apco25_sync[LEN_APCO25_SYNC] = {
// Sync = 5575F5FF77FF
// Or binary: 010101010111010111110101111111110111011111111111
               1,1,1,1,1,3,1,1,3,3,1,1,3,3,3,3,1,3,1,3,3,3,3,3	// or in symbols          
};


static const char apco25_duid_str[16][32] = {
	"Header Data Unit",	// 0
	"Reserved 1",		// 1
	"Reserved 2",			// 2
	"Terminator w/o Link Ctrl",	// 3
	"Reserved 4",			// 4
	"Logical Link Data Unit 1",	// 5
	"Reserved 6 (Astro VSELP?)",    // 6
	"Trunking Signaling Data Unit",	// 7
	"Reserved 8",			// 8
	"Reserved 9 (Astro VSELP?)",	// 9
	"Logical Link Data Unit 2",	// A
	"Reserved B",			// B
	"Packet Data Unit",		// C
	"Reserved D",			// D
	"Reserved E",			// E
	"Terminator with Link Ctrl"	// F
};

namespace gr {
  namespace fsk4 {

    class apco25_f_impl : public apco25_f
    {
     private:
        gr::msg_queue::sptr d_queue;		 


        int sym_counter;

        bool reverse_polarity;



        void framer(unsigned char sym);
        int framer_state;
        unsigned char framer_buffer[NFRAMER_BUFFER_APCO25];
        int iframer_buffer;
        int symbol_counter;

        void strip_status_syms(unsigned char sym);

        unsigned char data_unit[2048];

        int process_NID(unsigned char *dd);

        int process_HDU(unsigned char *dd);

        int apco25_nid_nac, apco25_nid_duid;

     public:
      apco25_f_impl(gr::msg_queue::sptr queue, int processing_flags);
      ~apco25_f_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace fsk4
} // namespace gr

#endif /* INCLUDED_FSK4_APCO25_F_IMPL_H */

