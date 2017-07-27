/* -*- c++ -*- */

#define FSK4_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "fsk4_swig_doc.i"

%{
#include "fsk4/demod_ff.h"
#include "fsk4/generic_f.h"
#include "fsk4/apco25_f.h"
#include "fsk4/rdlap_f.h"
%}


%include "fsk4/demod_ff.h"
GR_SWIG_BLOCK_MAGIC2(fsk4, demod_ff);
%include "fsk4/generic_f.h"
GR_SWIG_BLOCK_MAGIC2(fsk4, generic_f);
%include "fsk4/apco25_f.h"
GR_SWIG_BLOCK_MAGIC2(fsk4, apco25_f);
%include "fsk4/rdlap_f.h"
GR_SWIG_BLOCK_MAGIC2(fsk4, rdlap_f);
