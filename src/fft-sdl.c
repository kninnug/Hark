#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <SDL2/SDL.h>
#include <fftw3.h>
#include <portaudio.h>
#include <pthread.h>

#include "util.h"
#include "harmonics.h"

struct aBuf{
	size_t length;
	
	int samplerate;
	int fftWinInc;
	
	double * samples;
	
	fftw_plan panama;
	double * fftIn;
	fftw_complex * fftOut;
	
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

SDL_Renderer * initSDL(const char * title, SDL_Window ** outWin, int width, int height){
	SDL_Renderer * ret;
	
	if(SDL_Init(SDL_INIT_VIDEO)){
		fprintf(stderr, "! SDL_Init: %s\n", SDL_GetError());
		exit(-1);
		
		return NULL; // we won't reach here, but I like the visual hint
	}
	
	atexit(SDL_Quit);
	
	*outWin = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
	if(*outWin == NULL){
		fprintf(stderr, "! SDL_CreateWindow: %s\n", SDL_GetError());
		exit(-1);
		
		return NULL;
	}
	
	ret = SDL_CreateRenderer(*outWin, -1, SDL_RENDERER_SOFTWARE);
	if(ret == NULL){
		fprintf(stderr, "! SDL_CreateRenderer: %s\n", SDL_GetError());
		exit(-1);
		
		return NULL;
	}
	
	return ret;
}

PaStream * initPortAudio(struct aBuf * buf, PaStreamCallback callback){
	PaError paer;
	PaStream * stream;
	
	paer = Pa_Initialize();
	if(paer != paNoError){
		fprintf(stderr, "! Pa_Initialize: %s\n", Pa_GetErrorText(paer));
		exit(-1);
		
		return NULL;
	}
	
	paer = Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, buf->samplerate, buf->fftWinInc, callback, buf);
	if(paer != paNoError){
		fprintf(stderr, "! Pa_OpenDefaultStream: %s\n", Pa_GetErrorText(paer));
		exit(-1);
		
		return NULL;
	}
	
	return stream;
}

int recordCallback(const void * vin, void * vout, unsigned long frameCount,
	const PaStreamCallbackTimeInfo * timeInfo, PaStreamCallbackFlags statusFlags, void * vdata){
	
	struct aBuf * data = vdata;
	const float * in = vin;
	size_t i;
	
	for(i = 0; i < data->fftWinInc; i++){
		data->samples[i] = in[i]; // capture the first bit
	}
	
	pthread_mutex_lock(&data->mutex);
	
	for(i = data->fftWinInc; i < data->length; i += data->fftWinInc){
		memcpy(data->fftIn + i, data->samples, data->fftWinInc); // copy the first bit over and over
	}
	//memcpy(data->fftIn, data->samples, data->length);
	pthread_cond_signal(&data->cond);
	pthread_mutex_unlock(&data->mutex);
	
	return paContinue;
}

struct aBuf initABuf(int fftSize, int fftWinInc){
	struct aBuf buf;
	
	buf.length = fftSize;
	buf.fftWinInc = fftWinInc;
	buf.samplerate = 44100;
	
	buf.samples = fmalloc(buf.fftWinInc * sizeof *buf.samples);
	buf.fftIn = fmalloc(buf.length * sizeof *buf.fftIn);
	buf.fftOut = fmalloc(buf.length * sizeof *buf.fftOut);
	
	buf.mutex = PTHREAD_MUTEX_INITIALIZER;
	buf.cond = PTHREAD_COND_INITIALIZER;
	
	return buf;
}

void * fftThread(void * vdata){
	
	return NULL;
	
}

int main(int argc, char ** argv){
	int fftSize = 1024 * 32,
		fftWinInc = 1024 * 1,
		windowWidth = 640,
		windowHeight = 480;
		
	SDL_Window * win;
	SDL_Renderer * renderer = initSDL("FFT-SDL", &win, windowWidth, windowHeight);
	
	pthread_t ffThread1;
	
	struct aBuf buf = initABuf(1024 * 32, 1024 * 2);
	
	double freq, intens, hDiff;
	int harmonic = 0, octave = 0;
	const char * note;
	const char * line;
	size_t i, idx;
	int x, y;
	
	PaStream * stream = initPortAudio(&buf, recordCallback);
	
	buf.panama = fftw_plan_dft_r2c_1d(buf.length, buf.fftIn, buf.fftOut, FFTW_ESTIMATE);
	
	for(i = 0; i < buf.length; i++){
		buf.fftOut[i][0] = 0.0;
		buf.fftOut[i][1] = 0.0;
		buf.fftIn[i] = 0.0;
	}
	
	for(i = 0; i < buf.fftWinInc; i++){
		buf.samples[i] = 0.0;
	}
	
	genHarmonics();
	
	//pthread_create(&ffThread1, NULL, fftThread, &buf);
	
	printf("- Init done\n");
	
	Pa_StartStream(stream);
	
	pthread_mutex_lock(&buf.mutex);
	while(1){
		pthread_cond_wait(&buf.cond, &buf.mutex);
		while(!SDL_PollEvent(NULL)); // intenionally empty: poll ALL the events!
		
		fftw_execute(buf.panama);
		
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		for(i = 0; i < windowWidth; i++){
			/*harmonic = (double)(UMIN_HARMONIC + MAX_HARMONIC) * ((double)i / (double)windowWidth);
			freq = harmonicToFreq(harmonic - UMIN_HARMONIC);
			idx = freq / (double)buf.samplerate * ((double)buf.length / 2.0);
			y = (sqrt(buf.fftOut[idx][0]*buf.fftOut[idx][0] + buf.fftOut[idx][1]*buf.fftOut[idx][1]) / ((double)buf.length / 2.0)) * 2.0 * windowHeight;*/
			x = ((double)i / ((double)windowWidth)) * ((double)buf.length / 2.0 + 1);
			y = (sqrt(buf.fftOut[x][0]*buf.fftOut[x][0] + buf.fftOut[x][1]*buf.fftOut[x][1]) / ((double)buf.length / 2.0 + 1)) * windowHeight;
			SDL_RenderDrawLine(renderer, i, windowHeight, i, windowHeight - y);
		}
		SDL_RenderPresent(renderer);
		
		i = highFreq(buf.fftOut, buf.length, &intens);
		freq = (double)i/(double)buf.length*(double)buf.samplerate;
		
		harmonic = freqToHarmonic(freq < 16.0 ? 16.0 : freq, &hDiff);
		note = harmonicToNote(harmonic, &octave);
		line = harmonicToLine(harmonic);
		printf("%12.6f: (~%12.6f)   % 3i   %s%s%i   %s   %12.6f\n", 
			freq, harmonicToFreq(harmonic) - freq, harmonic, note, strlen(note) == 1 ? " " : "", octave, line, intens);
			
	}
	
	//while(Pa_IsStreamActive(stream)) Pa_Sleep(100);
	
	Pa_StopStream(stream);
	Pa_CloseStream(stream);
	
	//SDL_DestroyWindow(win);
	
	return 0;
}
