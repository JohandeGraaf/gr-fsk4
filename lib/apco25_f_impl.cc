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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "apco25_f_impl.h"

namespace gr {
  namespace fsk4 {

    apco25_f::sptr
    apco25_f::make(gr::msg_queue::sptr queue, int processing_flags)
    {
      return gnuradio::get_initial_sptr
        (new apco25_f_impl(queue, processing_flags));
    }

    /*
     * The private constructor
     */
    apco25_f_impl::apco25_f_impl(gr::msg_queue::sptr queue, int processing_flags)
      : gr::block("apco25_f",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(0, 0, 0)),
      d_queue(queue)
    {
      sym_counter = 0;


      reverse_polarity = false;
      if ((processing_flags & 0x01) != 0) reverse_polarity = true;

      iframer_buffer = 0;
      framer_state = 0;
      symbol_counter = 0;


      //  set_history (NTAPS + 2);	// we need to look at the previous values covering
    }

    /*
     * Our virtual destructor.
     */
    apco25_f_impl::~apco25_f_impl()
    {
    }

    void
    apco25_f_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
      // given output request of noutput_items return number of input items required
      int items = noutput_items; 		// this is 1:1 stub
      for (unsigned int i = 0; i < ninput_items_required.size(); i++)  ninput_items_required[i] = items; 
    }

    // -------------------------------------------------------------------------------

    int
    apco25_f_impl::process_HDU(unsigned char *dd)
    {
      int i;

      // TODO: Error correction


      // symbols 0-323 are HDU, 5 nulls should be starting at 324

      printf("HDU: ");

      for (i=0; i<5; i++) printf("%1i:",*(dd+i+324));
      printf("\n");

      return 0;
    }

    // -------------------------------------------------------------------------------

    int
    apco25_f_impl::process_NID(unsigned char *dd)
    {

      // TODO: Error correction


      // for now we can extract embedded data with hassling with error correction
      // systematic error correction quite simply is the dog's bollocks for lazy people


      apco25_nid_nac = *(dd);
      apco25_nid_nac = (apco25_nid_nac << 2) | *(dd+1);
      apco25_nid_nac = (apco25_nid_nac << 2) | *(dd+2);
      apco25_nid_nac = (apco25_nid_nac << 2) | *(dd+3);
      apco25_nid_nac = (apco25_nid_nac << 2) | *(dd+4);
      apco25_nid_nac = (apco25_nid_nac << 2) | *(dd+5);


      apco25_nid_duid = *(dd+6);
      apco25_nid_duid = (apco25_nid_duid << 2) | *(dd+7);

      printf("Network Access Code: %03X\tData Unit ID %1X: %s\n",apco25_nid_nac, apco25_nid_duid, apco25_duid_str[apco25_nid_duid]);

      return 0;
    }

    // -------------------------------------------------------------------------------
    //
    void
    apco25_f_impl::strip_status_syms(unsigned char sym)
    {
      static int raw_symbol_counter = 0;

      if (sym == (unsigned char) 255)
      {
        // remember... this gets triggered when sync symbols 0..23 inclusive are detected; 
        // next symbol will be index 24
        raw_symbol_counter = 24;
        symbol_counter = 0;

        // printf("\n");

        return;
      }

      

      if ( (raw_symbol_counter % 35) != 0)
      {
        //    printf("%1i",sym);

        data_unit[symbol_counter] = sym;
        if (symbol_counter < 2047) symbol_counter++;

        // 32 symbols completes incoming NID; process to get length of reset of data unit
        if (symbol_counter == 32) process_NID(&data_unit[0]);

        //    if ((symbol_counter == 361) && (apco25_nid_duid == 0)) process_HDU(&data_unit[32]);


      }
      else
      {
        // status bits are ostracized here

      }

      raw_symbol_counter++;

    }



    // -------------------------------------------------------------------------------
    // first step: detect frame sync sequence.
    // input data a stream of unsigned chars from set of 0, 1, 2, or 3
    // this routine strips out frame sync and passes all further symbols to higher level 
    void
    apco25_f_impl::framer(unsigned char sym)
    {
      int i,j, sync_mismatch;


      // framer_state 0 means waiting for sync sequence to appear 
      // (higher level routines can turn off feed as desired until resynced)
      if (framer_state != 0) strip_status_syms(sym);

      // place incoming sysmbol 
      framer_buffer[iframer_buffer] = sym;
      iframer_buffer = (iframer_buffer + 1) % NFRAMER_BUFFER_APCO25;

      sync_mismatch = 0;
      j = iframer_buffer;	// point to oldest sample in buffer
      for (i=0; i<LEN_APCO25_SYNC; i++)
      {
        if (framer_buffer[j] !=  apco25_sync[i]) sync_mismatch++;
        j = (j+1) % NFRAMER_BUFFER_APCO25;
      }

      if (sync_mismatch < 3)
      {
        strip_status_syms(255);		// signal reset 
        framer_state = 1;
      }

    }

    // -------------------------------------------------------------------------------
    // take our stream of incoming floats, convert to symbols and pass upwards
    //
    // Probably will never happen:  implement soft decision decoding
    //
    int
    apco25_f_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      unsigned char sym;

      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];

      // Do <+signal processing+>
      // run through all provided data; 
      // data has been scaled such that symbols should be at -3, -1, +1, or +3 
      for (int i = 0; i < noutput_items; i++)
      {
        // unlike RD-LAP we are gray coded
        if (in[i] < -2) sym = 3;
        else if (in[i] <  0.0) sym = 2;
        else if (in[i] <  2.0) sym = 0;
        else sym = 1;

        if (reverse_polarity) sym ^= 0x02;	// yes this is correct inversion for gray coded dibits

        framer(sym);
      }

      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.  (Nothing, ever!)
      return 0;
    }

  } /* namespace fsk4 */
} /* namespace gr */

