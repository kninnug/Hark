#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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
	double freq, intens, hDiff;
	int harmonic = 0, octave = 0;
	const char * note;
	const char * line;
	size_t i;
#ifdef MULTIFREQ
	int freqs[5] = {0};
	struct heap * h = heap_new(5);
#endif
	
	pthread_mutex_lock(&data->mutex);
	while(1){
		pthread_cond_wait(&data->cond, &data->mutex);
		
		fftw_execute(data->panama);
#ifdef MULTIFREQ
		threshFreq(data->fftOut, data->length, 1000, freqs, 5, h);
		
		
		for(i = 0; i < 5; i++){
			freq = (double)freqs[i]/(double)data->length*(double)data->samplerate;
			harmonic = freqToHarmonic(freq < 16.0 ? 16.0 : freq, &hDiff);
			note = harmonicToNote(harmonic, &octave);
			printf(" %12.6f % 3i %2s", freq, harmonic, note);
		}
		putchar('\n');
#else
		i = highFreq(data->fftOut, data->length, &intens);
		freq = (double)i/(double)data->length*(double)data->samplerate;
		
		//if(intens > 1000){
			harmonic = freqToHarmonic(freq < 16.0 ? 16.0 : freq, &hDiff);
			note = harmonicToNote(harmonic, &octave);
			line = harmonicToLine(harmonic);
			printf("%12.6f: (~%12.6f)   % 3i   %s%s%i   %s   %12.6f\n", 
				freq, harmonicToFreq(harmonic) - freq, harmonic, note, strlen(note) == 1 ? " " : "", octave, line, intens);
		//}
#endif
	}
	
	return NULL;
}

// do a sliding window
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

// capture only fftWinInc samples and replicate them
// should remove the delay, but I have no idea what the effect on the FFT is
// >> Seems alright
int recordCallback2(const void * vin, void * vout, unsigned long frameCount,
	const PaStreamCallbackTimeInfo * timeInfo, PaStreamCallbackFlags statusFlags, void * vdata){
	
	struct aBuf * data = vdata;
	const float * in = vin;
	size_t i;
	
	assert(frameCount == data->fftWinInc);
	assert(data->length % data->fftWinInc == 0);
	
	for(i = 0; i < frameCount; i++){
		data->samples[i] = data->amplifier * in[i]; // capture the first bit
	}
	
	for(i = data->fftWinInc; i < data->length; i += data->fftWinInc){
		memcpy(data->samples + i, data->samples, data->fftWinInc); // copy the first bit over and over
	}
	
	pthread_mutex_lock(&data->mutex);
	memcpy(data->fftIn, data->samples, data->length);
	pthread_cond_signal(&data->cond);
	pthread_mutex_unlock(&data->mutex);
	
	return paContinue;
}

int main(int argc, char ** argv){
	// FFT stuff
	int fftSize = 1024 * 48;
	int fftWinInc = 1024 * 2;
	// Portaudio stuff
	PaError paer;
	PaStream * stream;
	// PThread stuff
	pthread_t ffThread1;
	//pthread_t ffThread2; 
	
	struct aBuf buf;
	
	size_t i;
	
	buf.amplifier = 1.0;
	buf.length = fftSize;
	buf.samplerate = 44100;
	buf.fftWinInc = fftWinInc;
	
	if(argc > 1 && ((fftSize = strtoul(argv[1], NULL, 10)) != 0)){
		buf.length = fftSize;
	}
	if(argc > 2 && ((fftWinInc = strtoul(argv[2], NULL, 10)) != 0)){
		buf.fftWinInc = fftWinInc;
	}
	
	buf.pos = buf.length - buf.fftWinInc;
	
	buf.samples = fmalloc(buf.length * sizeof *buf.samples);
	buf.fftIn = fmalloc(buf.length * sizeof *buf.samples);
	buf.fftOut = fftw_malloc(buf.length * sizeof *buf.fftOut);
	
	buf.panama = fftw_plan_dft_r2c_1d(buf.length, buf.fftIn, buf.fftOut, FFTW_ESTIMATE);
	
	for(i = 0; i < buf.length; i++){
		buf.fftOut[i][0] = 0.0;
		buf.fftOut[i][1] = 0.0;
		buf.fftIn[i] = 0.0;
		
		buf.samples[i] = 0.0;
	}
	
	buf.mutex = PTHREAD_MUTEX_INITIALIZER;
	buf.cond = PTHREAD_COND_INITIALIZER;
	
	genHarmonics();
	
	paer = Pa_Initialize();
	if(paer != paNoError){
		fprintf(stderr, "! Pa_Initialize failed: %s\n", Pa_GetErrorText(paer));
		return EXIT_FAILURE;
	}
	paer = Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, buf.samplerate, fftWinInc, recordCallback2, &buf);
	if(paer != paNoError){
		fprintf(stderr, "! Pa_OpenDefaultStream failed: %s\n", Pa_GetErrorText(paer));
		return EXIT_FAILURE;
	}
	
	printf("---- ----\nInit done\n---- ----\n");
	
	printf("FFT-size: %i\nWindow-length: %f\nWindow-inc: %i\n", 
		buf.length, (double)buf.length/(double)buf.samplerate, buf.fftWinInc);
	
	pthread_create(&ffThread1, NULL, fftThread, &buf);
	//pthread_create(&ffThread2, NULL, fftThread, &buf);
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
