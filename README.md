Hark
====

Introduction
------------

These are my experiments into Fourier Transforms and Threading. My ultimate goal
with this is to be able to recognise whistled melodies. Speech recognition by
whistling instead of speaking. Should be much more reliable, I thought.

The general idea is this: record signal, FFT it, then grab the most intense
frequency. 

(Testing) Programs
------------------

There are currently three (testing) programs:

 - `fft-test` reads a file using libsndfile, runs an FFT over it and displays
    the frequencies.
 - `fft-record` records 3 seconds of sound using Portaudio, runs an FFT over it
    and displays the frequencies.
 - `fft-thread` is the most complex: it records continually and does the FFT'ing
    and displaying in a separate thread.
	
All the programs compile with GCC-4.8.1 under MinGW-32 on Windows 7. I use Dr.
Memory to check for memory-mistakes.

Dependencies
------------

 - `fft-test` uses libsndfile (http://www.mega-nerd.com/libsndfile/)
 - `fft-record` and `fft-thread` use PortAudio (http://portaudio.com/)
 - `fft-thread` uses Pthread(-w32) (http://www.sourceware.org/pthreads-win32/)
 
They all use FFTW 3 (http://fftw.org/)

License
-------

The license is a modified zlib license. I include no copyright notices in source
files since it's not legally necessary ('all rights reserved' is the default
copyright on every work, so noting it is redundant and deprecated). See LICENSE
for details. Basically I added a clause that includes NOTICE-files in this and
any derived work.

It is my understanding that this license is compatible with the licenses that
the dependencies use. If anyone believes otherwise let them speak (now) or.. 
later.
