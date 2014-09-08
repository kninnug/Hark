#ifndef HARKMONICS
#define HARKMONICS

#include <stdio.h>
#include <string.h>
#include <math.h>

#define UMIN_HARMONIC 48
#define MIN_HARMONIC -48
#define MAX_HARMONIC  83

void genHarmonics();

int freqToHarmonic(double freq, double * diff);

double harmonicToFreq(int harmonic);

const char * harmonicToNote(int harmonic, int * octave);

const char * harmonicToLine(int harmonic);

#endif