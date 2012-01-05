#include "blr_util.h"

double 
ts_delta(struct timeval *start, struct timeval *stop){
	return (stop->tv_sec + stop->tv_usec/1000000.0) - (start->tv_sec + start->tv_usec/1000000.0);
}

