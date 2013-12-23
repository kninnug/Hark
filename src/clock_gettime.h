#ifndef WINDOWS_CLOCK_GETTIME
#define WINDOWS_CLOCK_GETTIME

#include <sys/time.h>

int clock_gettime(int X, struct timeval *tv);

#endif