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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "demod_ff_impl.h"

namespace gr {
  namespace fsk4 {

    demod_ff::sptr
    demod_ff::make(gr::msg_queue::sptr queue, float sample_rate, float symbol_rate)
    {
      return gnuradio::get_initial_sptr
        (new demod_ff_impl(queue, sample_rate, symbol_rate));
    }

    /*
     * The private constructor
     */
    demod_ff_impl::demod_ff_impl(gr::msg_queue::sptr queue, float sample_rate, float symbol_rate)
      : gr::block("demod_ff",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(1, 1, sizeof(float))),
      d_queue(queue)
    {
      int i;

      sample_rate_hz = (double) sample_rate;
      symbol_rate_hz = (double) symbol_rate;

      last_input = 0;

      symbol_spread = 2.0;		// nominal symbol spread of 2.0 gives outputs at -3, -1, +1, +3
      symbol_time = 0.0;
      dt_symbol_time = symbol_rate_hz / sample_rate_hz;

      idata_history = 0;
      for (i=0; i<NTAPS; i++) data_history[i] = 0.0;



      fine_frequency_correction = 0.0;
      coarse_frequency_correction = 0.0;
    }

    /*
     * Our virtual destructor.
     */
    demod_ff_impl::~demod_ff_impl()
    {
    }

    void
    demod_ff_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      // ninput_items_required[0] = noutput_items;

      // given output request of noutput_items return number of input items required
      int items = noutput_items;
      for (unsigned int i = 0; i < ninput_items_required.size(); i++)      {
        ninput_items_required[i] = (int) (items * sample_rate_hz / symbol_rate_hz);
      }
    }


    // --------------------------------------------------------------------------


    void
    demod_ff_impl::send_frequency_correction()
    {
      double arg1, arg2;

      if (d_queue->full_p())		// if the queue is full, don't block, drop the data...
        return;

      if ((coarse_frequency_correction < COARSE_FREQUENCY_DEADBAND) && (coarse_frequency_correction > -COARSE_FREQUENCY_DEADBAND))
        return;

      arg1 = coarse_frequency_correction;
      arg2 = 0.0;

      // build & send a message
      gr::message::sptr msg = gr::message::make(0, arg1, arg2, 0); // vlen() * sizeof(float));
      //memcpy(msg->msg(), &d_max[0], vlen() * sizeof(float));
      d_queue->insert_tail(msg);
      msg.reset();
    }


    // ---------------------------------------------------------------------------


    bool
    demod_ff_impl::tracking_loop_mmse(float input, float*output)
    {
      int i, j;

      // update symbol clock
      symbol_time += dt_symbol_time;

      // update MMSE interpolator buffer
      data_history[idata_history] = input;
      idata_history = (idata_history + 1) % NTAPS;


      if (symbol_time > 1.0)
      {
        double interp, interp_p1, symbol_error;
        int imu, imu_p1;

        symbol_time -= 1.0;	// new symbol_time also tells us how far to interpolate relative to dt_symbol_time


        // at this point we state that linear interpolation was tried but found to be slightly
        // inferior.  Using MMSE interpolation shouldn't be a terrible burden

        imu = (int) floor(0.5 + (NSTEPS * ( (symbol_time / dt_symbol_time))));
        imu_p1 = imu + 1;
        if (imu >= NSTEPS) { imu_p1 = NSTEPS; imu = NSTEPS - 1;}
        
        j = idata_history;
        interp = 0.0;
        interp_p1 = 0.0;
        for (i=0; i<NTAPS; i++)
        {
          interp    +=  taps[imu   ][i] * data_history[j];
          interp_p1 +=  taps[imu_p1][i] * data_history[j];


          j = (j+1) % NTAPS;
        }


        // our output symbol will be interpolated value corrected for symbol_spread and frequency offset
        interp    -= fine_frequency_correction;
        interp_p1 -= fine_frequency_correction;

        // output is corrected for symbol deviation (spread)
        *output = 2.0 * interp / symbol_spread;
      

        // detect received symbol error: basically use a hard decision and subtract off expected position
        // nominal symbol level which will be +/- 0.5*symbol_spread and +/- 1.5 * symbol_spread
        // remember: nominal symbol_spread will be 2.0
        if (interp < - symbol_spread) {
            // symbol is -3: Expected at -1.5 * symbol_spread
            symbol_error = interp + (1.5 * symbol_spread);
            symbol_spread -= (symbol_error * 0.5 * K_SYMBOL_SPREAD);
        }
        else if (interp < 0.0)  {
            // symbol is -1: Expected at -0.5 * symbol_spread
            symbol_error = interp + (0.5 * symbol_spread);
            symbol_spread -= (symbol_error * K_SYMBOL_SPREAD);
        }
        else if (interp < symbol_spread) {
            // symbol is +1: Expected at +0.5 * symbol_spread
            symbol_error = interp - (0.5 * symbol_spread);
            symbol_spread += (symbol_error * K_SYMBOL_SPREAD);
        }
        else {
            // symbol is +3: Expected at +1.5 * symbol_spread
            symbol_error = interp - (1.5 * symbol_spread);
            symbol_spread += (symbol_error * 0.5 * K_SYMBOL_SPREAD);
        }


        if (interp_p1 < interp) {
            symbol_time += (symbol_error * K_SYMBOL_TIMING);  
        } else {
            symbol_time -= (symbol_error * K_SYMBOL_TIMING);
        }



        // it seems reasonable to constrain symbol spread to +/- 20% of nominal 2.0
        if (symbol_spread > SYMBOL_SPREAD_MAX) {
            symbol_spread = SYMBOL_SPREAD_MAX; 
        } else if (symbol_spread < SYMBOL_SPREAD_MIN) {
            symbol_spread = SYMBOL_SPREAD_MIN;
        }
        

        
        // coarse tracking loop: for eventually frequency shift request generation
        coarse_frequency_correction += (( fine_frequency_correction - coarse_frequency_correction) * K_COARSE_FREQUENCY);

        // fine loop 
        fine_frequency_correction += (symbol_error * K_FINE_FREQUENCY);


        return true;
      }


      return false;
    }


    // ---------------------------------------------------------------------------


    int
    demod_ff_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      int noutputs_produced = 0;

      const float *in = (const float *) input_items[0];
      float *out = (float *) output_items[0];

      // Do <+signal processing+>
      // first we run through all provided data
      for (int i = 0; i < noutput_items; i++)
      {
        if ( tracking_loop_mmse(in[i], &out[noutputs_produced]) ) noutputs_produced++;
      }

      // send frequency adjusment request if needed 
      send_frequency_correction();

      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (noutput_items);

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace fsk4 */
} /* namespace gr */

