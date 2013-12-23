#include <stdlib.h>
#include <stdio.h>

#include <fftw3.h>
#include <portaudio.h>
#include <pthread.h>

#include "util.h"
#include "harmonics.h"

struct aBuf{
	size_t length;
	size_t pos;
	int samplerate;
	int fftWinInc;
	fftw_plan panama;
	double * samples;
	double * fftIn;
	fftw_complex * fftOut;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	double amplifier;
};

void * fftThread(void * vdata){
	struct aBuf * data = vdata;
	double freq, intens;
	int harmonic = 0, octave = 0;
	const char * note;
	const char * line;
	
	pthread_mutex_lock(&data->mutex);
	while(1){
		pthread_cond_wait(&data->cond, &data->mutex);
		
		fftw_execute(data->panama);
		freq = highFreq(data->fftOut, data->length, data->samplerate, &intens);
		
		if(freq < 16.0) freq = 16.0;
		
		harmonic = freqToHarmonic(freq);
		note = harmonicToNote(harmonic, &octave);
		line = harmonicToLine(harmonic);
		printf("%12.6f:   % 3i   %s%s%i   %s   %12.6f\n", 
			freq, harmonic, note, strlen(note) == 1 ? " " : "", octave, line, intens);
	}
	
	return NULL;
}

int recordCallback(const void * vin, void * vout, unsigned long frameCount, 
	const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void * vdata){
	
	struct aBuf * data = vdata;
	const float * in = vin;
	size_t i;
	
	for(i = 0; i < frameCount; i++){
		if(data->pos == data->length) break;
		if(data->pos > data->length) return paAbort;
		data->samples[data->pos] = data->amplifier * in[i];
		data->pos++;
	}
	// either the buffer is full or we ran out of input
	if(data->pos == data->length){
		/*if(pthread_mutex_trylock(&data->mutex)){ // FFT'ing took too long, bail
			// fprintf(stderr, "! recordCallback's trylock failed\n");
			return paAbort;
		}*/
		pthread_mutex_lock(&data->mutex);
		memcpy(data->fftIn, data->samples, data->length);
		pthread_cond_signal(&data->cond);
		pthread_mutex_unlock(&data->mutex);
		
		// shift data
		data->pos -= data->fftWinInc; // == data->length - data->fftWinInc
		memmove(data->samples, data->samples + data->fftWinInc, data->pos * sizeof *data->samples);
	}
	
	// in case we haven't run out of input yet
	for(; i < frameCount; i++){
		if(data->pos >= data->length) break;
		data->samples[data->pos] = data->amplifier * in[i];
		data->pos++;
	}
	
	return paContinue;
}

int main(int argc, char ** argv){
	// FFT stuff
	int fftSize = 1024 * 8;
	int fftWinInc = fftSize / 4;
	// Portaudio stuff
	PaError paer;
	PaStream * stream;
	// PThread stuff
	pthread_t ffThread1;
	//pthread_t ffThread2; 
	
	size_t i;
	
	struct aBuf buf;
	buf.length = fftSize;
	buf.pos = 0;
	buf.samplerate = 44100;
	buf.fftWinInc = fftWinInc;
	
	buf.samples = fmalloc(buf.length * sizeof *buf.samples);
	for(i = 0; i < buf.length; i++) buf.samples[i] = 0.0;
	buf.fftIn = fmalloc(buf.length * sizeof *buf.samples);
	for(i = 0; i < buf.length; i++) buf.fftIn[i] = 0.0;
	buf.fftOut = fftw_malloc(buf.length * sizeof *buf.fftOut);
	for(i = 0; i < buf.length; i++){
		buf.fftOut[i][0] = 0.0;
		buf.fftOut[i][1] = 0.0;
	}
	
	buf.panama = fftw_plan_dft_r2c_1d(buf.length, buf.fftIn, buf.fftOut, FFTW_ESTIMATE);
	
	buf.mutex = PTHREAD_MUTEX_INITIALIZER;
	buf.cond = PTHREAD_COND_INITIALIZER;
	
	buf.amplifier = 1.0;
	if(argc > 1 && ((buf.amplifier = strtod(argv[1], NULL)) < 1.0)){
		buf.amplifier = 1.0;
	}
	
	printf("---- ----\nInit done\n---- ----\n");
	
	printf("FFT-size: %i\nWindow-length: %f\nWindow-inc: %i\n", fftSize, (double)fftSize/(double)buf.samplerate, fftWinInc);
	
	pthread_create(&ffThread1, NULL, fftThread, &buf);
	//pthread_create(&ffThread2, NULL, fftThread, &buf);
	
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
	Pa_Terminate();
	free(buf.samples);
	free(buf.fftIn);
	fftw_free(buf.fftOut);
	fftw_destroy_plan(buf.panama);
	
	return 0;
}
