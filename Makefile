ALL_LIBS = -lfftw3 -lportaudio -lwinmm
STD_OPTS = -Wall -pedantic -ggdb -D_ISOC99_SOURCE -std=c99

all: fft-test

harmonica: harmonics.h harmonics.c
	gcc -DHARKMONIC_MAIN $(STD_OPTS) -o harmonics harmonics.c

fft-test: fft-test.c harmonics.o util.o
	gcc $(STD_OPTS) -o fft-test fft-test.c $(ALL_LIBS) harmonics.o util.o -lsndfile
	
fft-record: fft-record.c harmonics.o util.o
	gcc $(STD_OPTS) -o fft-record fft-record.c $(ALL_LIBS) harmonics.o util.o
	
fft-thread: fft-thread.c harmonics.o util.o
	gcc $(STD_OPTS) -o fft-thread fft-thread.c $(ALL_LIBS) harmonics.o util.o -pthread
	
clock_gettime.o: clock_gettime.h clock_gettime.c
	gcc $(STD_OPTS) -o clock_gettime.o -c clock_gettime.c
	
harmonics.o: harmonics.h harmonics.c
	gcc $(STD_OPTS) -o harmonics.o -c harmonics.c
	
util.o: util.h util.c
	gcc $(STD_OPTS) -o util.o -c util.c