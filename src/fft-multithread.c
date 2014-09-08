#include <stdlib.h>
#include <stdio.h>

#include <fftw3.h>
#include <portaudio.h>
#include <pthread.h>

#include "util.h"
#include "harmonics.h"

struct fftBuf;

struct aBuf{
	int samplerate;
	int fftSize;
	int fftWinInc;
	double amplifier;
	
	size_t length;
	size_t pos;
	double * samples;
	
	size_t numThreads;
	struct fftBuf * ffts;
};

struct fftBuf{
	fftw_plan panama;
	double * fftIn;
	fftw_complex * fftOut;
	
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int threadId;
	
	struct aBuf * info;
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
	size_t i, j;
	size_t numThreads = 2;
	// FFT stuff
	int fftSize = 1024 * 4;
	int fftWinInc = fftSize / 4;
	// Portaudio stuff
	PaError paer;
	PaStream * stream;
	
	struct fftBuf ffts[numThreads];
	struct aBuf buf = {44100, fftSize, fftWinInc, 1.0, fftSize, 0, NULL, numThreads, ffts};
	
	buf.samples = fmalloc(buf.length * sizeof *buf.samples);
	for(j = 0; j < buf.length; j++) buf.samples[j] = 0.0;
	
	for(i = 0; i < numThreads; i++){
		ffts[i].fftIn = fmalloc(ffts[i].length * sizeof *ffts[i].samples);
		for(j = 0; j < ffts[i].length; j++) ffts[i].fftIn[j] = 0.0;
		ffts[i].fftOut = fftw_malloc(ffts[i].length * sizeof *ffts[i].fftOut);
		for(j = 0; j < ffts[i].length; j++){
			ffts[i].fftOut[j][0] = 0.0;
			ffts[i].fftOut[j][1] = 0.0;
		}
		
		ffts[i].panama = fftw_plan_dft_r2c_1d(buf.length, buf.fftIn, buf.fftOut, FFTW_ESTIMATE);
		
		ffts[i].mutex = PTHREAD_MUTEX_INITIALIZER;
		ffts[i].cond = PTHREAD_COND_INITIALIZER;
		
		ffts[i].info = &buf;
	}
	
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
