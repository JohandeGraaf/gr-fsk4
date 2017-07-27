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
#include "generic_f_impl.h"

namespace gr {
  namespace fsk4 {

    generic_f::sptr
    generic_f::make(gr::msg_queue::sptr queue, int processing_flags)
    {
      return gnuradio::get_initial_sptr
        (new generic_f_impl(queue, processing_flags));
    }

    /*
     * The private constructor
     */
    generic_f_impl::generic_f_impl(gr::msg_queue::sptr queue, int processing_flags)
      : gr::block("generic_f",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(0, 0, 0)),
      d_queue(queue)
    {
        sym_counter = 0;


        reverse_polarity = false;
        if ((processing_flags & 0x01) != 0) reverse_polarity = true;
    }

    /*
     * Our virtual destructor.
     */
    generic_f_impl::~generic_f_impl()
    {
    }

    void
    generic_f_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
        // given output request of noutput_items return number of input items required
        int items = noutput_items; 		// this is 1:1 stub
        for (unsigned int i = 0; i < ninput_items_required.size(); i++)  ninput_items_required[i] = items;
    }

    // -------------------------------------------------------------------------------
    // take our stream of incoming floats, convert to symbols and pass upwards
    //
    // Probably will never happen:  implement soft decision decoding
    //
    int
    generic_f_impl::general_work (int noutput_items,
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
        if (in[i] < -2) sym = 0;
        else if (in[i] <  0.0) sym = 1;
        else if (in[i] <  2.0) sym = 2;
        else sym = 3;

        if (reverse_polarity) sym ^= 0x03;

        printf("%c",0x41+sym);

        sym_counter++;
        if (sym_counter >= 80)
        {
          sym_counter = 0;
          printf("\n");
        }


      }

      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.  (Nothing, ever!)
      return 0;
    }

  } /* namespace fsk4 */
} /* namespace gr */

