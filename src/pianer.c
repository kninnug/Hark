/*
 * Pianer: synthesize piano sounds
 * Inspiration: http://taradov.com/piano.php
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <assert.h>

#include "portaudio.h"

#ifndef M_PI
#define M_PI 3.1415926538
#endif

const int SAMPLERATE = 44100;
const int PLAYTIME = 9; // in seconds
const double AMPLITUDEEPSILON = 1e-6;

struct reverbParams{
	size_t delay;
	double drop;
	double rate;
};

struct noteParams{
	double a; // initial amplitude
	double g; // decay rate
	double f; // frequency
	
	size_t pos;
	struct reverbParams * reverb;
	
	double t;
	double _t;
	double _e;
	double _a;
};

struct aData{
	size_t numNotes;
	struct noteParams * notes; // these MUST be ordered by noteParams.pos
	
	size_t position;
};

int playCallback( const void *vin, void *vout, unsigned long frames, 
		const PaStreamCallbackTimeInfo* timeInfo, 
		PaStreamCallbackFlags status, void *vdata ){
	
	struct aData * data = vdata;
	struct noteParams * note;
	float * out = vout;
	double t, s, r, a;
	size_t i, j;
	
	for(i = 0; i < frames; i++, data->position++){
		out[i] = 0.0;
		t = (double)data->position / (double)SAMPLERATE;
		
		// sum(i = 0 -> N){ a[i] * e^(-g[i] * t) * sin(2 * pi * f[i] * t) }
		// phase is ignored
		for(j = 0; j < data->numNotes; j++){
			note = data->notes + j;
			s = (double)note->pos / (double)SAMPLERATE;
			r = note->pos + note->reverb->delay - data->position;
			
			// since notes MUST be ordered by pos we can break as soon as we find one that is ahead
			if(note->pos > data->position) break;
			
			// decay per note must not depend on its position
			a = note->a * exp(-note->g * (t - s)); 
			
			if(r > 0){
				a *= note->reverb->drop * note->reverb->rate * (double)r;
			}
			
			// this note has 'died out', don't waste cycles on the sin(). 
			if(a < AMPLITUDEEPSILON) continue; 
			
			// It'd be nice if we could filter the note out entirely
			// Perhaps this could be pre-computed, using .g and .a
			// a * e^(-g * (t - s)) < EPSILON
			// a * e^(-g * (t - s)) = EPSILON
			note->_e = a;
			// e^(-g * (t - s)) = EPSILON / a
			// -g * (t - s) = ln(EPSILON / a)
			// t - s = ln(EPSILON / a) / -g
			// t = ln(EPSILON / a) / -g + s
			note->_t = log(AMPLITUDEEPSILON / note->a) / -note->g + s;
			note->t = t;
			note->_a = a;
			// t > ln(EPSILON / a) / -g + s
			
			out[i] += a * sin(2 * M_PI * note->f * t);
		}
	}
	
	return data->position >= SAMPLERATE * PLAYTIME ? paComplete : paContinue;
}

int main(int argc, char ** argv){
	PaError paer = Pa_Initialize();
	PaStream * stream;
	struct reverbParams reverbs = {SAMPLERATE / 10, 1.0, 0.1};
	struct noteParams notes[] = {
		{0.8, 4.0, 440.0, 0, &reverbs},
		{0.1, 1.2, 441.0, 0, &reverbs},
		{0.1, 1.1, 441.2, 0, &reverbs},
		
		{0.8, 4.0, 220.0, SAMPLERATE, &reverbs},
		{0.1, 1.2, 221.0, SAMPLERATE, &reverbs},
		{0.1, 1.1, 221.2, SAMPLERATE, &reverbs},
		
		{0.8, 4.0, 880.0, 2*SAMPLERATE, &reverbs},
		{0.1, 1.2, 881.0, 2*SAMPLERATE, &reverbs},
		{0.1, 1.1, 881.2, 2*SAMPLERATE, &reverbs}
	};
	struct aData data = {sizeof notes / sizeof *notes, notes, 0};
	size_t i;
	
	if(paer != paNoError){
		fprintf(stderr, "! Pa_Initialize: %s\n", Pa_GetErrorText(paer));
		exit(-1);
	}
	
	paer = Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, SAMPLERATE, 
			paFramesPerBufferUnspecified, playCallback, &data);
	if(paer != paNoError){
		fprintf(stderr, "! Pa_OpenDefaultStream: %s\n", Pa_GetErrorText(paer));
		exit(-1);
	}
	
	printf("- Init done\n");
	
	Pa_StartStream(stream);
	
	while(Pa_IsStreamActive(stream)){
		Pa_Sleep(100);
	}
	
	for(i = 0; i < sizeof notes / sizeof *notes; i++){
		printf("@ %i: %f / %f : %f / %f : %f\n", i, notes[i]._e, 
				AMPLITUDEEPSILON, notes[i]._t, notes[i].t, notes[i]._a);
	}
	
	Pa_CloseStream(stream);
	if(paer != paNoError){
		fprintf(stderr, "! Pa_CloseStream: %s\n", Pa_GetErrorText(paer));
		exit(-1);
	}
	
	Pa_Terminate(); // I don't care if this fails, we're bailing anyway
	return 0;
}
