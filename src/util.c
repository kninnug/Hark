#include "util.h"

void * fmalloc(size_t n){
	void * p = malloc(n);
	if(p == NULL){
		fprintf(stderr, "! malloc failed (%zu)\n", n);
		exit(EXIT_FAILURE);
	}
	return p;
}

double highFreq(fftw_complex * fftOut, int fftSize, int samplerate, double * intens){
	size_t i;
	double freq, high = 0, amp = 0;
	
	for(i = 0; i < fftSize/2 + 1; i++){
		amp = fftOut[i][0]*fftOut[i][0] + fftOut[i][1]*fftOut[i][1];
		if(amp > high){
			high = amp;
			freq = (double)i/(double)fftSize*(double)samplerate;
		}
		// freq = (double)i/(double)fftSize*(double)samplerate;
		// printf("(#%4i): %f %f\n", i, freq, fftOut[i][0]*fftOut[i][0] + fftOut[i][1]*fftOut[i][1]);
	}
	
	if(intens != NULL) *intens = high;
	
	return freq;
}