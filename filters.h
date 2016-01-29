/**
This is a modified version of ftools.h provided by Richard Dobson as part of "The Audio Programming Book"
*/

/* Copyright (c) 2009 Richard Dobson

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

/* ftools.h : a set of first/second order recursive filters */
/* SOURCES:

  (1) Robert Bristow-Johnson "EQ Cookbook"  (ref. music-dsp.org )

  (2) Dodge & Jerse "Computer Music" 2nd Ed, pp 209-219

	All filter tick functions support time-varying parameters.
	There is no attempt here to optimise the code!

  Functions are provided to enable display of filter coefficients,
  and to create a filter using externally supplied coefficients.
  No time-varying control possible in this case!
*/

#ifndef FILTERS_H_INCLUDED
#define FILTERS_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_PI
#define M_PI  (3.1415926535897932)
#endif

#ifndef TWOPI
#define TWOPI (2.0 * M_PI)
#endif

#include <stdio.h>

typedef enum	bw_filtertype {LOWPASS, HIPASS, BANDPASS, BANDREJECT} FILTERTYPE;

typedef struct bq_coeffs
{
	double a0,a1,a2,b1,b2;
} BQCOEFFS;

/* Dodge & Jerse Butterworth filters */
typedef  void (*bw_updatefunc)(BQCOEFFS* coeffs, double cutoff, double bw,double pidivsr,double twopidivsr);


typedef struct butterworth_filter
{
	BQCOEFFS coeffs;
	double x1,x2,y1,y2;
	double cutoff,srate,bandwidth;
	double pidivsr,twopidivsr;
	FILTERTYPE ftype;
	bw_updatefunc func;
} BUTTERWORTH;




BQCOEFFS get_bw_coefficients(const BUTTERWORTH* filter);

BUTTERWORTH* new_filter();
int		bw_init(BUTTERWORTH* flt, FILTERTYPE ftype, unsigned long srate,double cutoff,double bandwidth);
int	bw_tick(BUTTERWORTH* flt, int input, double cutoff,double bandwidth);

static void bw_update_lp(BQCOEFFS* coeffs, double cutoff, double bandwidth,double pidivsr,double twopidivsr);
static void bw_update_hp(BQCOEFFS* coeffs, double cutoff, double bandwidth,double pidivsr,double twopidivsr);
static void bw_update_bp(BQCOEFFS* coeffs, double cutoff, double bandwidth,double pidivsr,double twopidivsr);
static void bw_update_br(BQCOEFFS* coeffs, double cutoff, double bandwidth,double pidivsr,double twopidivsr);

void filtResponse(BUTTERWORTH* bwf, double* resp, double* phase);

#ifdef __cplusplus
}
#endif

#endif // FILTERS_H_INCLUDED
