#include <sys/time.h>
#include <stdio.h>
#ifndef BLR_UTIL_H
#define BLR_UTIL_H
double ts_delta(struct timeval *start, struct timeval *stop);
FILE * safe_mkstemp(const char *hostname, const char *slurm_job_name, int rank);
void get_env_int(const char *name, int *val, int defaultVal);
#define max(a,b) ((a)>=(b)?(a):(b))
#endif // BLR_UTIL_H

