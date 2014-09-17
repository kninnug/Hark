ALL_LIBS = -lfftw3 -lportaudio -lwinmm harmonics.o util.o
STD_OPTS = -Wall -pedantic -ggdb -D_ISOC99_SOURCE -std=c99

all: fft-thread

harmonica: harmonics.h harmonics.c
	gcc -DHARKMONIC_MAIN $(STD_OPTS) -o harmonics harmonics.c

fft-test: fft-test.c harmonics.o util.o
	gcc $(STD_OPTS) -o fft-test fft-test.c $(ALL_LIBS) -lsndfile
	
fft-record: fft-record.c harmonics.o util.o
	gcc $(STD_OPTS) -o fft-record fft-record.c $(ALL_LIBS)
	
fft-thread: fft-thread.c harmonics.o util.o
	gcc $(STD_OPTS) -o fft-thread fft-thread.c $(ALL_LIBS) -pthread
	
fft-multithread: fft-multithread.c harmonics.o util.o
	gcc $(STD_OPTS) -o fft-multithread fft-multithread.c $(ALL_LIBS) -pthread

fft-sdl: fft-sdl.c harmonics.o util.o
	gcc $(STD_OPTS) -o fft-sdl fft-sdl.c $(ALL_LIBS) -pthread -lm -mconsole `sdl2-config --libs`

pianer: pianer.c
	gcc $(STD_OPTS) -o pianer pianer.c -lm -lportaudio -lwinmm
	
clock_gettime.o: clock_gettime.h clock_gettime.c
	gcc $(STD_OPTS) -o clock_gettime.o -c clock_gettime.c
	
harmonics.o: harmonics.h harmonics.c
	gcc $(STD_OPTS) -o harmonics.o -c harmonics.c
	
util.o: util.h util.c
	gcc $(STD_OPTS) -o util.o -c util.c
	
clean:
	rm -f *.o
	rm -f *.exe
	rm -f *~