/**
This is a modified version of ftools.c provided by Richard Dobson as part of "The Audio Programming Book"
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

/* ftools.c : a set of first/second order recursive filters */
/* SOURCES:

  (1) Robert Bristow-Johnson "EQ Toolkit"  (music-dsp.org )

  (2) Dodge & Jerse "Computer Music" 2nd Ed, pp 209-219

	All filter tick functions support time-varying parameters.
	There is no attempt here to optimise the code!

  Functions are provided to enable display of filter coefficients,
  and to create a filter using externally supplied coefficients.
  No time-varying control possible in this case - but filter will run fast.
*/
/* NB: code adds gain compensation via "ampscale" */
/* BIG TODO: replace BW switch block with func ptrs! */

#import "filters.h"
#import <math.h>
#import <stdio.h>

BUTTERWORTH* new_filter()
{
	BUTTERWORTH* bwf = NULL;
	BQCOEFFS coeffs;
	bwf = (BUTTERWORTH* ) malloc(sizeof(BUTTERWORTH));
	if(bwf == NULL)
		return NULL;
	coeffs.a0 = coeffs.a1 = coeffs.a2 = coeffs.b1 = coeffs.b2 = bwf->x1 = bwf->x2 = bwf->y1 = bwf->y2 = 0.0;
	bwf->bandwidth = bwf->cutoff =  bwf->srate = bwf->pidivsr = bwf->twopidivsr = 0.0;
	bwf->coeffs = coeffs;
	return bwf;
}

BQCOEFFS get_bw_coefficients(const BUTTERWORTH* filter)
{
	BQCOEFFS coeffs = {0.0};
	if(filter)
	    return filter->coeffs;
	return coeffs;
}

int bw_init(BUTTERWORTH* bwf, FILTERTYPE ftype, unsigned long srate,double cutoff,double bandwidth)
{
	if(bwf == NULL)
		return -1;
	bwf->srate = (double) srate;
	bwf->cutoff = cutoff;
	bwf->bandwidth = bandwidth;
	bwf->pidivsr = M_PI / bwf->srate;
	bwf->twopidivsr = 2.0 * bwf->pidivsr;
	bwf->ftype = ftype;
	switch(ftype){
	case(LOWPASS):
		bwf->func = bw_update_lp;
		break;
	case(HIPASS):
		bwf->func = bw_update_hp;
		break;
	case(BANDPASS):
		bwf->func = bw_update_bp;
		break;
	case(BANDREJECT):
		bwf->func = bw_update_br;
		break;
	default:
		return -1;
	}
	bwf->func(&bwf->coeffs,cutoff,bandwidth,bwf->pidivsr,bwf->twopidivsr);
	return 0;
}

void bw_update_lp(BQCOEFFS* coeffs, double cutoff, double bandwidth,double pidivsr,double twopidivsr)
{
	double C;
	C =  1.0 / tan(pidivsr * cutoff);
	coeffs->a0 = 1.0 / (1 + sqrt(2) * C + (C * C));
	coeffs->a1 = 2.0 * coeffs->a0;
	coeffs->a2 = coeffs->a0;
	coeffs->b1 = 2.0 * coeffs->a0 * (1.0 - (C*C));
	coeffs->b2 = coeffs->a0 * (1.0 - sqrt(2) * C + (C * C));
}

void bw_update_hp(BQCOEFFS* coeffs, double cutoff, double bandwidth,double pidivsr,double twopidivsr)
{
	double C;
	C =  tan(pidivsr * cutoff);
	coeffs->a0 = 1.0 / (1 + sqrt(2) * C + (C * C));
	coeffs->a1 = -2.0 * coeffs->a0;
	coeffs->a2 = coeffs->a0;
	coeffs->b1 = 2.0 * coeffs->a0 * ((C*C) - 1.0);
	coeffs->b2 = coeffs->a0 * (1.0 - sqrt(2) * C + (C * C));
}

void bw_update_bp(BQCOEFFS* coeffs, double cutoff, double bandwidth,double pidivsr,double twopidivsr)
{
	double C,D;

	C =  1.0 / tan(pidivsr * bandwidth);
	D = 2.0 * cos(twopidivsr * cutoff);
	coeffs->a0 = 1.0 / (1.0 + C);
	coeffs->a1 = 0.0;
	coeffs->a2 = - coeffs->a0;
	coeffs->b1 = - coeffs->a0 * C * D;
	coeffs->b2 = coeffs->a0 * (C - 1.0);
}


void bw_update_br(BQCOEFFS* coeffs, double cutoff, double bandwidth,double pidivsr,double twopidivsr)
{
	double C,D;

	C =  tan(pidivsr * bandwidth);
	D =  2.0 * cos(twopidivsr * cutoff);
	coeffs->a0 = 1.0 / (1.0 + C);
	coeffs->a1 = -coeffs->a0 * D;;
	coeffs->a2 = coeffs->a0;
	coeffs->b1 = -coeffs->a0 * D;
	coeffs->b2 = coeffs->a0 * (1.0 - C);
}

void filtResponse(BUTTERWORTH* bwf, double* resp, double* phase){
	BQCOEFFS coeffs = get_bw_coefficients(bwf);
	int i;
	int sampleRate=1024;
	for(i=0;i<sampleRate;i++){
		double imagX, imagY, realX, realY;
		imagX=-1*(coeffs.a1*sin(i*M_PI/sampleRate)+coeffs.a2*sin(2*i*M_PI/sampleRate));
		realX=(coeffs.a0 + coeffs.a1*cos(i*M_PI/sampleRate) + coeffs.a2*cos(2*i*M_PI/sampleRate));

		imagY=-1*(coeffs.b1*sin(i*M_PI/sampleRate)+coeffs.b2*sin(2*i*M_PI/sampleRate));
		realY=(1 + coeffs.b1*cos(i*M_PI/sampleRate) + coeffs.b2*cos(2*i*M_PI/sampleRate));
		phase[i] = atan2(imagX,realX) -atan2(imagY,realY);

		resp[i]=sqrt((imagX*imagX + realX*realX)/(imagY*imagY + realY*realY));
	}
}


int bw_tick(BUTTERWORTH* bwf, int input, double cutoff,double bandwidth)
{
	double output;

	if(!(bwf->cutoff == cutoff && bwf->bandwidth == bandwidth)){
		bwf->cutoff = cutoff;
		bwf->bandwidth = bandwidth;
		bwf->pidivsr = M_PI / bwf->srate;
		bwf->twopidivsr = 2.0 * bwf->pidivsr;
		/* update coefficients */
		bwf->func(&bwf->coeffs,cutoff,bandwidth,bwf->pidivsr,bwf->twopidivsr);
	}
	output = bwf->coeffs.a0 * input + bwf->coeffs.a1 * bwf->x1 + bwf->coeffs.a2 * bwf->x2
				- bwf->coeffs.b1 * bwf->y1 - bwf->coeffs.b2 * bwf->y2;
	bwf->x2 = bwf->x1;	bwf->x1 = input;
	bwf->y2 = bwf->y1;	bwf->y1 = output;
	output+=0.5;
	return (int) output;
}
