#ifndef HARK_UTIL_H
#define HARK_UTIL_H

#include <stdio.h>
#include <stdlib.h>

#include "fftw3.h"

void * fmalloc(size_t n);

double highFreq(fftw_complex * fftOut, int fftSize, int samplerate, double * intens);

#endif