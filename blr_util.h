#include <sys/time.h>
#include <stdio.h>
#ifndef BLR_UTIL_H
#define BLR_UTIL_H
double ts_delta(struct timeval *start, struct timeval *stop);
FILE * safe_mkstemp(int rank, const char *str);
#endif // BLR_UTIL_H

