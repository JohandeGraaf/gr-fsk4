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
#include "rdlap_f_impl.h"

namespace gr {
  namespace fsk4 {

    rdlap_f::sptr
    rdlap_f::make(gr::msg_queue::sptr queue, int processing_flags)
    {
      return gnuradio::get_initial_sptr
        (new rdlap_f_impl(queue, processing_flags));
    }

    /*
     * The private constructor
     */
    rdlap_f_impl::rdlap_f_impl(gr::msg_queue::sptr queue, int processing_flags)
      : gr::block("rdlap_f",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(0, 0, 0)),
      d_queue(queue)
    {
      int i;


      framer_state = 0;
      iframer_buffer = 0;
      for (i=0; i<NFRAMER_BUFFER; i++) framer_buffer[i] = 0;

      accumulate_block_index = 0; 

      reverse_polarity = false;
      if ((processing_flags & 0x01) != 0) reverse_polarity = true;

      //  set_history (NTAPS + 2);	// we need to look at the previous values covering
    }

    /*
     * Our virtual destructor.
     */
    rdlap_f_impl::~rdlap_f_impl()
    {
    }

    void
    rdlap_f_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
      // given output request of noutput_items return number of input items required
      int items = noutput_items; 		// this is 1:1 stub
      for (unsigned int i = 0; i < ninput_items_required.size(); i++)  ninput_items_required[i] = items; 
    }



    // -------------------------------------------------------------------------------
    // CRC2 check, for header block
    // Good enough implementation (TM) 
    //
    int rdlap_f_impl::test_CRC2(unsigned char *dd)
    {
      int i,j,crc,ma, acrc;
      
      acrc = *(dd+10);
      acrc = (acrc << 8) | (*(dd + 11));
      crc = 0;

      for (i=0; i<10; i++)
      {
        ma = 0x80;
        for (j=0; j<8; j++)
        {
          if (( *(dd+i) & ma) != 0) crc ^= 0x8000;
          crc = crc << 1;

          if ((crc & 0x10000) != 0) crc ^= 0x11021;

          ma = ma >> 1;
        }
      }

      crc = crc ^ 0xFFFF;


      //NumTotal ++;
      if (crc == acrc) 
      {
      //NumGood++; 
      return 1; 
      }

      return 0;
    }


    // -------------------------------------------------------------------------------
    // Viterbi error correction for rate 3/4 TCM with 8 state (3 bit) memory
    // Good enough implementation (TM) - Not cleaned up for instructional or example
    // purposes but has been verified as functionally correct; probably not optimum
    // implementation either
    //
    // You probably should be looking at GNU Radio's viterbi code


    int StateTransition[8][8]={
      {0x9, 0x3, 0x1, 0xB, 0xC, 0x6, 0x4, 0xE},
      {0x1, 0xB, 0xC, 0x6, 0x4, 0xE, 0x9, 0x3},
      {0x5, 0xF, 0xD, 0x7, 0x0, 0xA, 0x8, 0x2},
      {0xD, 0x7, 0x0, 0xA, 0x8, 0x2, 0x5, 0xF},
      {0x0, 0xA, 0x8, 0x2, 0x5, 0xF, 0xD, 0x7},
      {0x8, 0x2, 0x5, 0xF, 0xD, 0x7, 0x0, 0xA},
      {0xC, 0x6, 0x4, 0xE, 0x9, 0x3, 0x1, 0xB},
      {0x4, 0xE, 0x9, 0x3, 0x1, 0xB, 0xC, 0x6}
    };


    struct BranchStruct {
      int StateHistory[0x80];
      int metric;
    };


    struct BranchStruct Branch[8];
    int nElements;

    // Returns squared Euclidian Distance between 4x4 constellation points s1 & s2
    int rdlap_f_impl::viterbi_SED(int s1, int s2)
    {
      int d1,d2,ss;

      d1 = ((s1 & 0x03) - (s2 & 0x03));
      if (d1 < 0) d1 = -d1;
      d2 = ( (s1 >> 2) - (s2 >> 2));
      if (d2 < 0) d2 = -d2;

      ss = (d1*d1) + (d2*d2);

    //  printf("SED %1X %1X (%i,%i):\t%i\n",s1,s2,d1,d2,ss);
      return ss;
    }

    void rdlap_f_impl::viterbi_reset(void)
    {

      int i;
      for (i=0; i<8; i++)
      {
        Branch[i].metric = 0;
      }
      nElements = 0;
    }


    void rdlap_f_impl::viterbi_add_data(int cc)
    {
      int i,j, iwin, sedwin, sedtemp;
      int iTarget;
      int Winner[8];
      int SedWinner[8];
      int TempData[8][0x80];

      if (nElements == 0)
      {
        // first go around: None of the branches have been filled
        // fill them with the eight possible target states starting from initial state zero
        for (i=0; i<8; i++)
        {
      Branch[i].metric = viterbi_SED(StateTransition[0][i], cc);
      Branch[i].StateHistory[nElements] = i;
        }
      }
      else
      {
        // All subsequent state have a branch going to each node...

        // let's go through each target state and find the min SED index
        
        for (iTarget = 0; iTarget < 8; iTarget++)
        {
          // start with def
          iwin = 0;
          sedwin = viterbi_SED(StateTransition[0][iTarget] , cc);	// SED: symbol for state 0 to iTarget, actual
          sedwin += Branch[0].metric;

          for (i=1; i<8; i++)
          {
            sedtemp = viterbi_SED(StateTransition[i][iTarget] , cc);
            sedtemp += Branch[i].metric;

      if (sedtemp < sedwin)
            {
        sedwin = sedtemp;
        iwin = i;
            }

          }

          Winner[iTarget] = iwin;
          SedWinner[iTarget] = sedwin;
        }

        // Now need to update Branches:  first backup all original data
        for (i=0; i<8; i++)
        {
          for (j=0; j<nElements; j++)
          {
            TempData[i][j] = Branch[i].StateHistory[j];
          }
        }

        // now can create new branches...
        for (i=0; i<8; i++)
        {
          Branch[i].metric = SedWinner[i];
          iwin = Winner[i];
          for (j=0; j<nElements; j++)
          {
            Branch[i].StateHistory[j] = TempData[iwin][j];
          }

          // and new element
          Branch[i].StateHistory[nElements] = i;

        }


      }



      nElements++;

    }


    int rdlap_f_impl::viterbi_return_data(unsigned char *data)
    {
      int i, nub, thb, di;
      
      nub = 0;
      di = 0;
      thb = 0;
      for (i=0; i<nElements; i++)
      {
        thb = (thb << 3) | Branch[0].StateHistory[i];
        nub += 3;
        if (nub >= 8)
        {
          *(data+di) =  (thb >> (nub - 8)) & 0xff;
          di ++;
          nub -= 8;
        }

      }

      // Any remaining partials go into last
      *(data+di) = thb & ( (1 << nub) - 1);

      return (3*nElements);

    }







    // -------------------------------------------------------------------------------
    // this includes 3 status symbols @ positions 22, 45, 68 that are ignored
    void
    rdlap_f_impl::process_block()
    {
      int i,j;
      unsigned char data[0x20];
      unsigned char symbol_pair;
      static int blocks_in_PDU;


      viterbi_reset();

      for (i=0; i<33; i++)
      {
        // deinterleave data block and send to viterbi error correction routine
        j = block_interleave[i];
        symbol_pair = accumulate_block_buffer[j];
        symbol_pair = (symbol_pair << 2) | accumulate_block_buffer[j+1];
      
        //if (processed_blocks_since_sync == 0) printf("%1X",symbol_pair);

        viterbi_add_data(symbol_pair);
      }

      viterbi_return_data(&data[0]);

      // first block??? let's grab number of expected blocks in this PDU
      if (processed_blocks_since_sync == 0)
      {
        blocks_in_PDU = data[0x06] & 0x7F;
        if (test_CRC2(&data[0]) == 0) 
        {
          //framer_state = 0;
          blocks_in_PDU = -1;
        }
        else printf("\n");
        // printf(" Blocks in PDU: %i\n",blocks_in_PDU);
      }



      if (processed_blocks_since_sync <= blocks_in_PDU)
      {
        for (i=0; i<12; i++) printf("%02X ",data[i] & 0xff);
        printf("\t");

        for (i=0; i<12; i++) 
        {
          if ( (data[i] & 0x7f) < 0x20) printf("."); else printf("%c",data[i] & 0x7f);
        }
    //    printf("\t%lg",omag);
        printf("\n");

      }
      //else framer_state = 0;


    }

    // -------------------------------------------------------------------------------
    // all higher level processing takes place in blocks of 69 symbols each...  
    void
    rdlap_f_impl::accumulate_block(unsigned char sym)
    {
      if (sym == 255)
      {
        // reset...  
        processed_blocks_since_sync = 0;

        accumulate_block_index = 0;
        return;
      }

      accumulate_block_buffer[accumulate_block_index] = sym;
      accumulate_block_index++;
      if (accumulate_block_index >= 69)
      {
        // ready to set up block processing

        process_block();
        processed_blocks_since_sync++;

        accumulate_block_index = 0; 
      }

    }

    // -------------------------------------------------------------------------------
    // first step: detect frame sync sequence.
    // input data a stream of unsigned chars from set of 0, 1, 2, or 3
    // this routine strips out frame sync and station id slots (total of 48 symbols) and
    // passes all further symbols to higher level 
    void
    rdlap_f_impl::framer(unsigned char sym)
    {
      int i,j, sync_mismatch;


      // framer_state 0 means waiting for sync sequence to appear 
      // (higher level routines can turn off feed as desired until resynced)
      if (framer_state != 0) accumulate_block(sym);

      // place incoming sysmbol 
      framer_buffer[iframer_buffer] = sym;
      iframer_buffer = (iframer_buffer + 1) % NFRAMER_BUFFER;

      sync_mismatch = 0;
      j = iframer_buffer;	// point to oldest sample in buffer
      for (i=0; i<LEN_RDLAP_SYNC; i++)
      {
        if (framer_buffer[j] !=  rdlap_sync[i]) sync_mismatch++;
        j = (j+1) % NFRAMER_BUFFER;
      }

      if (sync_mismatch < 3)
      {
        accumulate_block(255);		// signal reset 
        framer_state = 1;
      }

    }



    // -------------------------------------------------------------------------------
    // take our stream of incoming floats, convert to symbols and pass upwards
    //
    // Probably will never happen:  implement soft decision decoding
    //
    int
    rdlap_f_impl::general_work (int noutput_items,
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

