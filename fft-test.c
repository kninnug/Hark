#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "fftw3.h"
#include "sndfile.h"

#include "clock_gettime.h"
#include "harmonics.h"
#include "util.h"

void FFTandPrint(fftw_plan panama, fftw_complex * fftOut, int fftSize, int samplerate, size_t i){
	double freq;
	int harmonic; 
	const char * note;
	
	fftw_execute(panama);
	freq = highFreq(fftOut, fftSize, samplerate, NULL);
	harmonic = freqToHarmonic(freq);
	note = harmonicToNote(harmonic, NULL);
	printf("#%4i: f = %f -> %i (%s)\n", i, freq, harmonic, note == NULL ? "" : note);
}

void readAndFFT(fftw_plan panama, double * samples, fftw_complex * fftOut, SNDFILE * sndHandle, int samplerate, int fftSize, int windowInc){
	// int halfSize = fftSize / 2 + 1;
	sf_count_t itemsRead = 0;
	size_t i = 0;
	
	// first read full size, then increments of half-size to do windowy stuff
	itemsRead = sf_read_double(sndHandle, samples, fftSize);
	if(itemsRead < fftSize){
		fprintf(stderr, "! sf_read_double read too few items to do even one FFT (%llu)\n", itemsRead);
		exit(EXIT_FAILURE);
		return;
	}
	
	FFTandPrint(panama, fftOut, fftSize, samplerate, i);
	
	memmove(samples, samples + windowInc, (fftSize - windowInc) * sizeof *samples);
	
	while((itemsRead = sf_read_double(sndHandle, samples + (fftSize - windowInc), windowInc)) == windowInc){
		i++;
		FFTandPrint(panama, fftOut, fftSize, samplerate, i);
		
		memmove(samples, samples + windowInc, (fftSize - windowInc) * sizeof *samples);
	}
}

int main(int argc, char ** argv){
	// sndfile stuff
	SNDFILE * sndHandle;
	SF_INFO sndInfo = {0};
	// fft stuff
	int fftSize = 1024 * 4;
	fftw_plan panama; // a man, a plan a canal: panama!
	fftw_complex * fftOut;
	
	double * samples;
	const char * fileName = "440.wav";
	
	if(argc > 1){
		fileName = argv[1];
	}
	
	printf("File: %s\n", fileName);
	
	sndHandle = sf_open(fileName, SFM_READ, &sndInfo);
	if(sndHandle == NULL){
		fprintf(stderr, "! sf_open failed: %s\n", sf_strerror(sndHandle));
		return EXIT_FAILURE;
	}
	
	printf("File info:\n\tframes: %llu\n\tsample-rate: %i\n\tchannels: %i\n\tformat: %i\n\tsections: %i\n\tseekable: %i\n", 
		sndInfo.frames, sndInfo.samplerate, sndInfo.channels, sndInfo.format, sndInfo.sections, sndInfo.seekable);
		
	printf("Audio info:\n\tlength: %us\n", sndInfo.frames/sndInfo.samplerate);
	
	if(sndInfo.channels > 1){
		fprintf(stderr, "! Can only process mono sound (%i)\n", sndInfo.channels);
		return EXIT_FAILURE;
	}
	
	samples = fmalloc(fftSize * sizeof *samples);
	//memset(samples, 0, fftSize * sizeof *samples);
	fftOut = fftw_malloc(fftSize * sizeof *fftOut);
	//memset(fftOut, 0, fftSize * sizeof *fftOut);
	if(fftOut == NULL){
		fprintf(stderr, "! fftw_malloc failed (%i)\n", fftSize * sizeof *samples);
		return EXIT_FAILURE;
	}
	panama = fftw_plan_dft_r2c_1d(fftSize, samples, fftOut, FFTW_ESTIMATE);
	
	printf("FFT-size: %i\nWindow-width = %fs\n", fftSize, (double)fftSize/(double)sndInfo.samplerate);
	
	genHarmonics();
	
	sf_seek(sndHandle, 0, SEEK_SET);
	
	readAndFFT(panama, samples, fftOut, sndHandle, sndInfo.samplerate, fftSize, fftSize / 4);
	
	fftw_destroy_plan(panama);
	fftw_free(fftOut);
	free(samples);
	sf_close(sndHandle);
	return 0;
}
