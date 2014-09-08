#ifndef HARK_UTIL_H
#define HARK_UTIL_H

#include <stdio.h>
#include <stdlib.h>

#include "fftw3.h"

struct heap{
	size_t length;
	size_t front;
	double * items;
	int * addons;
};

void * fmalloc(size_t n);

size_t highFreq(fftw_complex * fftOut, int fftSize, double * intens);

int * threshFreq(fftw_complex * fftOut, int fftSize, int threshold, int * in, size_t len, struct heap * hin);

struct heap * heap_new(size_t length);

#endif