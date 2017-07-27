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


#ifndef INCLUDED_FSK4_DEMOD_FF_H
#define INCLUDED_FSK4_DEMOD_FF_H

#include <fsk4/api.h>
#include <gnuradio/block.h>
#include <gnuradio/msg_queue.h>
#include <gnuradio/message.h>

namespace gr {
  namespace fsk4 {

    /*!
     * \brief <+description of block+>
     * \ingroup fsk4
     *
     */
    class FSK4_API demod_ff : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<demod_ff> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of fsk4::demod_ff.
       *
       * To avoid accidental use of raw pointers, fsk4::demod_ff's
       * constructor is in a private implementation
       * class. fsk4::demod_ff::make is the public interface for
       * creating new instances.
       */
      static sptr make(gr::msg_queue::sptr queue, float sample_rate, float symbol_rate);
    };

  } // namespace fsk4
} // namespace gr

#endif /* INCLUDED_FSK4_DEMOD_FF_H */

