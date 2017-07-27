#!/usr/bin/env python

#
# Copyright 2006,2007 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 


#
# This application has been built on code primarily based on 
# GNU Radio modules usrp_flex.py.  
# 



from gnuradio import gr, gru, usrp, optfir, eng_notation, blks
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import time, os, sys
from string import split, join
from gnuradio import fsk4
from math import pi


"""
This example application demonstrates receiving and demodulating
Four Level FSK signals from the USRP or a file

The following are optional command line parameters:

-f FREQ	    USRP receive frequency (N/A if processing data file)
-p PROTOCOL Protocol Selection: 
		0 = 19.2KBaud RDLAP (Default)
		1 = APCO25 
-R SUBDEV   Daughter board specification, defaults to first found
-F FILE     Read samples from a file instead of USRP.
-d DECIM    Decimation rate on USRP FPGA (default 256)
            Used for live data and file
-c FREQ     Calibration offset.  Defaults to 0.0 Hz
            Added to USRP receive frequency OR input file offset       
-g GAIN     Daughterboard gain setting. Defaults to mid-range.
-l          Log flow graph to files (LOTS of data)
-v          Verbose output

Once the program is running, ctrl-break (Ctrl-C) stops operation.
"""

class app_top_block(gr.top_block):
    def __init__(self, options, queue):
        gr.top_block.__init__(self, "usrp_flex")
        self.options = options
	self.offset = 0.0
	self.file_offset = 0.0
	self.adj_time = time.time()
	self.verbose = options.verbose
			
	if options.from_file is None:
            # Set up USRP source with specified RX daughterboard
            self.src = usrp.source_c()
            if options.rx_subdev_spec == None:
                options.rx_subdev_spec = usrp.pick_rx_subdevice(self.src)
            self.subdev = usrp.selected_subdev(self.src, options.rx_subdev_spec)
            self.src.set_mux(usrp.determine_rx_mux_value(self.src, options.rx_subdev_spec))

            # set FPGA decimation rate
            self.src.set_decim_rate(options.decim)
	    	    
            # If no gain specified, set to midrange
            if options.gain is None:
                g = self.subdev.gain_range()
                options.gain = (g[0]+g[1])/2.0
            self.subdev.set_gain(options.gain)

            # Tune daughterboard
            actual_frequency = options.frequency+options.calibration
            tune_result = usrp.tune(self.src, 0, self.subdev, actual_frequency)
            if not tune_result:
                sys.stderr.write("Failed to set center frequency to "+`actual_frequency`+"\n")
                sys.exit(1)

            if options.verbose:
                print "Using RX daughterboard", self.subdev.side_and_name()
                print "USRP gain is", options.gain
                print "USRP tuned to", actual_frequency
            
        else:
            # Use supplied file as source of samples
	    self.file_offset = options.calibration
            print "File input offset", self.offset
            self.src = gr.file_source(gr.sizeof_gr_complex, options.from_file)
            if options.verbose:
                print "Reading samples from", options.from_file
	    
        if options.log and not options.from_file:
            usrp_sink = gr.file_sink(gr.sizeof_gr_complex, 'usrp.dat')
            self.connect(self.src, usrp_sink)

        # following is rig to allow fast and easy swapping of signal configuration
        # blocks between this example and the oscope example which uses self.msgq
	self.msgq = queue


        #-------------------------------------------------------------------------------


        if options.protocol == 0:

  	  # ---------- RD-LAP 19.2 kbps (9600 ksps), 25kHz channel, 
          self.symbol_rate = 9600				# symbol rate; at 2 bits/symbol this corresponds to 19.2kbps
 	  self.channel_decimation = 10				# decimation (final rate should be at least several symbol rate)
	  self.max_frequency_offset = 12000.0			# coarse carrier tracker leash, ~ half a channel either way
	  self.symbol_deviation = 1200.0			# this is frequency offset from center of channel to +1 / -1 symbols
          self.input_sample_rate = 64e6 / options.decim		# for USRP: 64MHz / FPGA decimation rate	
	  self.protocol_processing = fsk4.rdlap_f(self.msgq, 0)	# desired protocol processing block selected here

          self.channel_rate = self.input_sample_rate / self.channel_decimation

          # channel selection filter characteristics
          channel_taps = optfir.low_pass(1.0,   # Filter gain
                               self.input_sample_rate, # Sample rate
                               10000, # One-sided modulation bandwidth
                               12000, # One-sided channel bandwidth
                               0.1,   # Passband ripple
                               60)    # Stopband attenuation

	  # symbol shaping filter characteristics
          symbol_coeffs = gr.firdes.root_raised_cosine (1.0,            	# gain
                                          self.channel_rate ,       	# sampling rate
                                          self.symbol_rate,             # symbol rate
                                          0.2,                		# width of trans. band
                                          500) 				# filter type 


        if options.protocol == 1:

	  # ---------- APCO-25 C4FM Test Data
          self.symbol_rate = 4800					# symbol rate; at 2 bits/symbol this corresponds to 19.2kbps
	  self.channel_decimation = 20				# decimation
	  self.max_frequency_offset = 6000.0			# coarse carrier tracker leash 
	  self.symbol_deviation = 600.0				# this is frequency offset from center of channel to +1 / -1 symbols
          self.input_sample_rate = 64e6 / options.decim		# for USRP: 64MHz / FPGA decimation rate	
	  self.protocol_processing = fsk4.apco25_f(self.msgq, 0)
          self.channel_rate = self.input_sample_rate / self.channel_decimation


          # channel selection filter
          channel_taps = optfir.low_pass(1.0,   # Filter gain
                               self.input_sample_rate, # Sample rate
                               5000, # One-sided modulation bandwidth
                               6500, # One-sided channel bandwidth
                               0.1,   # Passband ripple
                               60)    # Stopband attenuation

	  # symbol shaping filter
          symbol_coeffs = gr.firdes.root_raised_cosine (1.0,            	# gain
                                          self.channel_rate ,       	# sampling rate
                                          self.symbol_rate,             # symbol rate
                                          0.2,                		# width of trans. band
                                          500) 				# filter type 



        # ---------- End of configuration 

	
	if options.verbose:
	    print "Channel filter has", len(channel_taps), "taps."


        self.chan = gr.freq_xlating_fir_filter_ccf(self.channel_decimation ,    # Decimation rate
                                              channel_taps,  # Filter taps
                                              0.0,   # Offset frequency
                                              self.input_sample_rate) # Sample rate


	# also note: 
        # this specifies the nominal frequency deviation for the 4-level fsk signal
	self.fm_demod_gain = self.channel_rate / (2.0 * pi * self.symbol_deviation)
        self.fm_demod = gr.quadrature_demod_cf (self.fm_demod_gain)
        symbol_decim = 1
        self.symbol_filter = gr.fir_filter_fff (symbol_decim, symbol_coeffs)

        # eventually specify: sample rate, symbol rate
        self.demod_fsk4 = fsk4.demod_ff(self.msgq, self.channel_rate , self.symbol_rate)


	if options.log:
	    chan_sink = gr.file_sink(gr.sizeof_gr_complex, 'chan.dat')
	    self.connect(self.chan, chan_sink)


        if options.log:
          chan_sink2 = gr.file_sink(gr.sizeof_float, 'demod.dat')
          self.connect(self.demod_fsk4, chan_sink2)

	self.connect(self.src, self.chan, self.fm_demod, self.symbol_filter, self.demod_fsk4, self.protocol_processing)





    def adjust_freq_norm(self, frequency_offset):
        self.offset -=  frequency_offset * self.symbol_deviation    
        if self.offset > self.max_frequency_offset:
          self.offset = self.max_frequency_offset
        if self.offset < -self.max_frequency_offset:
          self.offset = -self.max_frequency_offset
        print "Channel frequency offset message processed, now at (Hz):", int(self.offset)
        self.chan.set_center_freq(self.offset + self.file_offset)

#---------------------------------------------------------------------------------

	    		
def main():
    parser = OptionParser(option_class=eng_option)
    parser.add_option("-f", "--frequency", type="eng_float", default=None,
                      help="set receive frequency to Hz", metavar="Hz")
    parser.add_option("-R", "--rx-subdev-spec", type="subdev",
                      help="select USRP Rx side A or B", metavar="SUBDEV")
    parser.add_option("-d", "--decim", type="int", default=256,
                      help="set fgpa decimation rate to DECIM [default=%default]")
    parser.add_option("-p", "--protocol", type="int", default=0,
                      help="set protocol: 0 = RDLAP 19.2kbps (default); 1 = APCO25")
    parser.add_option("-c",   "--calibration", type="eng_float", default=0.0,
                      help="set frequency offset to Hz", metavar="Hz")
    parser.add_option("-g", "--gain", type="int", default=None,
                      help="set RF gain", metavar="dB")
    parser.add_option("-l", "--log", action="store_true", default=False,
                      help="log flowgraph to files (LOTS of data)")
    parser.add_option("-v", "--verbose", action="store_true", default=False,
                      help="display debug output")
    parser.add_option("-F", "--from-file", default=None,
                      help="read samples from file instead of USRP")
    (options, args) = parser.parse_args()

    if len(args) > 0 or (options.frequency == None and options.from_file == None):
	print "Run 'usrp_fsk4.py -h' for options."
	sys.exit(1)

    if options.frequency == None:
	options.frequency = 0.0

    # Flow graph emits pages into message queue
    queue = gr.msg_queue()
    tb = app_top_block(options, queue)
    
    try:
        tb.start()
	while 1:
	    if not queue.empty_p():
		msg = queue.delete_head() # Blocking read; _nowait would do nonblocking
	        frequency_correction = msg.arg1()
                tb.adjust_freq_norm(frequency_correction)


    except KeyboardInterrupt:
        tb.stop()

if __name__ == "__main__":
    main()
