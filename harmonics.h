#ifndef HARKMONICS
#define HARKMONICS

#include <stdio.h>
#include <string.h>
#include <math.h>

void genHarmonics();

int freqToHarmonic(double freq);

double harmonicToFreq(int harmonic);

const char * harmonicToNote(int harmonic, int * octave);

const char * harmonicToLine(int harmonic);

#endif