#include "harmonics.h"

static const char * notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

static const char * lines[] = {
	"  | | | | |  ", // anything else
	"O | | | | |  ", // C 4
	"# | | | | |  ", // C#4
	" O| | | | |  ", // D 4
	" #| | | | |  ", // D#4
	"  O | | | |  ", // E 4
	"  |O| | | |  ", // F 4
	"  |#| | | |  ", // F#4
	"  | O | | |  ", // G 4
	"  | # | | |  ", // G#4
	"  | |O| | |  ", // A 4
	"  | |#| | |  ", // A#4
	"  | | O | |  ", // B 4
	"  | | |O| |  ", // C 5
	"  | | |#| |  ", // C#5
	"  | | | O |  ", // D 5
	"  | | | # |  ", // D#5
	"  | | | |O|  ", // E 5
	"  | | | | O  ", // F 5
	"  | | | | |O ", // G 5
	"  | | | | |# ", // G#5
	"  | | | | | O", // A 5
	"  | | | | | #", // A#5
};

static double harmonics[UMIN_HARMONIC + MAX_HARMONIC];

const char * uharmonicToNote(size_t harmonic, int * octave){
	if(harmonic > MAX_HARMONIC + UMIN_HARMONIC) return "";
	
	if(octave != NULL) *octave = harmonic / 12;
	
	return notes[harmonic % (sizeof notes / sizeof notes[0])];
}

const char * harmonicToNote(int harmonic, int * octave){
	if(harmonic < MIN_HARMONIC || harmonic > MAX_HARMONIC) return "";
	
	if(harmonic < 0) harmonic = UMIN_HARMONIC + harmonic;
	
	return uharmonicToNote(harmonic, octave);
}

const char * harmonicToLine(int harmonic){
	if(harmonic < 0 || harmonic > 21) return lines[0];
	
	return lines[1 + harmonic];
}

double harmonicToFreq(int harmonic){
	if(harmonic < MIN_HARMONIC || harmonic > MAX_HARMONIC){
		return 0.0;
	}
	
	harmonic += UMIN_HARMONIC;
	
	return harmonics[harmonic];
}

/**
 * x = 12*log2(f/440)
 */
int freqToHarmonic(double freq, double * diff){
	double harm = (double)12*log2(freq/(double)440) + (double)9;
	int x = round(harm);
	double freq0 = harmonicToFreq(x-1),
		freq1 = harmonicToFreq(x),
		freq2 = harmonicToFreq(x+1),
		diff0 = fabs(freq0 - freq),
		diff1 = fabs(freq1 - freq),
		diff2 = fabs(freq2 - freq),
		nodiff = 0.0;
		
	if(diff == NULL) diff = &nodiff;
	//*diff = harm;
	//printf("f = %f, x = %3i, d@[x-1] = %f, d@[x] = %f, d@[x+1] = %f\n", freq, x, diff0, diff1, diff2);
	
	*diff = freq0;
	if(diff0 < diff1 && diff0 < diff2) return x - 1;
	*diff = freq1;
	if(diff1 < diff0 && diff1 < diff2) return x;
	*diff = freq2;
	if(diff2 < diff0 && diff2 < diff1) return x + 1;
	
	*diff = freq1;
	return x;
}

/**
 * f = 440*2^(x/12)
 * where x in [-48, 83]
 */
void genHarmonics(){
	int x;
	
	for(x = 0; x < UMIN_HARMONIC + MAX_HARMONIC + 1; x++){
		harmonics[x] = 440*pow(2.0, (double)(x - 9 - UMIN_HARMONIC)/12);
	}
}

#ifdef HARKMONIC_MAIN
int main(int argc, char ** argv){
	genHarmonics();
	return 0;
}
#endif
