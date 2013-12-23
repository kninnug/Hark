#include <stdlib.h>
#include <stdio.h>

#include "fftw3.h"
#include "portaudio.h"

#include "harmonics.h"
#include "util.h"

struct aBuf{
	int samplerate;
	size_t duration;
	size_t length;
	size_t pos;
	double * samples;
};

int recordCallback(const void * vin, void * vout, unsigned long frameCount, 
	const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void * vdata){
	
	struct aBuf * data = vdata;
	const float * in = vin;
	size_t i;
	
	if(data->pos + frameCount >= data->length) return paComplete;
	
	for(i = 0; i < frameCount; i++){
		data->samples[data->pos++] = in[i];
	}
	
	return paContinue;
}

void doFFT(struct aBuf * buf, fftw_plan panama, double * fftIn, fftw_complex * fftOut, int fftSize, int windowInc){
	size_t i = 0;
	double freq;
	int harmonic;
	
	while(i*windowInc + fftSize < buf->pos){
		memcpy(fftIn, buf->samples + i*windowInc, fftSize * sizeof *fftIn);
		
		fftw_execute(panama);
		
		freq = highFreq(fftOut, fftSize, buf->samplerate, NULL);
		harmonic = freqToHarmonic(freq);
		printf("#%4i@%i - %i: f = %f -> %i (%s)\n", i, i*windowInc, i*windowInc + fftSize, freq, harmonic, harmonicToNote(harmonic, NULL));
		
		i++;
	}
}

int main(int argc, char ** argv){
	// fft stuff
	int fftSize = 1024 * 4;
	int fftWinInc = fftSize / 4;
	fftw_plan panama; // a man, a plan a canal: panama!
	fftw_complex * fftOut;
	// portaudio stuff
	PaError paer;
	PaStream * stream;
	
	double * fftIn = fmalloc(fftSize * sizeof *fftIn);
	struct aBuf buf = {44100, 3, 3*44100, 0, NULL};
	buf.samples = fmalloc(buf.length * sizeof *buf.samples);
	
	fftOut = fftw_malloc(fftSize * sizeof *fftOut);
	if(fftOut == NULL){
		fprintf(stderr, "! fftw_malloc failed (%i)\n", fftSize * sizeof *fftOut);
		return EXIT_FAILURE;
	}
	panama = fftw_plan_dft_r2c_1d(fftSize, fftIn, fftOut, FFTW_ESTIMATE);
	
	paer = Pa_Initialize();
	if(paer != paNoError){
		fprintf(stderr, "! Pa_Initialize failed: %s\n", Pa_GetErrorText(paer));
		return EXIT_FAILURE;
	}
	paer = Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, buf.samplerate, fftWinInc, recordCallback, &buf);
	if(paer != paNoError){
		fprintf(stderr, "! Pa_OpenDefaultStream failed: %s\n", Pa_GetErrorText(paer));
		return EXIT_FAILURE;
	}
	paer = Pa_StartStream(stream);
	if(paer != paNoError){
		fprintf(stderr, "! Pa_StartStream failed: %s\n", Pa_GetErrorText(paer));
		return EXIT_FAILURE;
	}
	
	while(Pa_IsStreamActive(stream)) Pa_Sleep(100);
	
	Pa_CloseStream(stream);
	
	printf("FFT-size: %i (%f sec)\nWindow-width: %i\npos %zu / %zu\n", 
		fftSize, (double)fftSize/buf.samplerate, fftWinInc, buf.pos, buf.length);
	
	doFFT(&buf, panama, fftIn, fftOut, fftSize, fftWinInc);
	
	Pa_Terminate();
	fftw_destroy_plan(panama);
	fftw_free(fftOut);
	free(buf.samples);
	free(fftIn);
	
	return 0;
}
