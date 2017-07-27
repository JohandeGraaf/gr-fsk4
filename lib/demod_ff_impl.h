/* -*- c++ -*- */
/* 
 * Copyright 2017 Free Software Foundation, Inc.
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

#ifndef INCLUDED_FSK4_DEMOD_FF_IMPL_H
#define INCLUDED_FSK4_DEMOD_FF_IMPL_H

#include <fsk4/demod_ff.h>

// Following are gnuradio interpolator taps for mmse; 
#include "interpolator_taps.h"

// carrier deviation tracking related
static const double K_SYMBOL_SPREAD = 0.0100;	// tracking loop gain constant
// leashes on symbol spread (deviation); nominal value 2.0
static const double SYMBOL_SPREAD_MAX = 2.4;	// upper range limit: +20%
static const double SYMBOL_SPREAD_MIN = 1.6;    // lower range limit: -20%

// symbol clock tracking loop gain
static const double K_SYMBOL_TIMING = 0.025;

// carrier frequency offset tracking related
static const double K_FINE_FREQUENCY = 0.125;		// internal fast loop (must be this high to acquire symbol sync)
static const double K_COARSE_FREQUENCY = 0.00125;	// time constant for coarse tracking loop that send messages outside this block (should be relatively slow)
static const double COARSE_FREQUENCY_DEADBAND = 1.66;	// gnuradio frequency adjust messages will not be emitted until we exceed this thershold


// please note all inputs are post FM demodulator and symbol shaping filter
// data is normalized before being sent to this block so these parameters should
// not need adjusting even when working on different signals.
//
// nominal levels are -3, -1, +1, and +3 
//

namespace gr {
  namespace fsk4 {

    class demod_ff_impl : public demod_ff
    {
     private:
      gr::msg_queue::sptr d_queue;

      void send_frequency_correction();

      bool tracking_loop_mmse(float input, float*output);



      double sample_rate_hz, symbol_rate_hz;

      float last_input;
      double symbol_time, dt_symbol_time;
      double symbol_spread;

      int idata_history;
      float data_history[NTAPS];


      double fine_frequency_correction;
      double coarse_frequency_correction;

     public:
      demod_ff_impl(gr::msg_queue::sptr queue, float sample_rate, float symbol_rate);
      ~demod_ff_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace fsk4
} // namespace gr

#endif /* INCLUDED_FSK4_DEMOD_FF_IMPL_H */

