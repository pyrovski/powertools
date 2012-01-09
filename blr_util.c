#include <stdlib.h>		// mkstmp
#include <unistd.h>		// close
#include "blr_util.h"

double 
ts_delta(struct timeval *start, struct timeval *stop){
	return (stop->tv_sec + stop->tv_usec/1000000.0) - (start->tv_sec + start->tv_usec/1000000.0);
}

FILE *
safe_mkstmp( int rank, const char* str ){
	FILE *f;
	int fd=-1;
	char filename[1024];
	sprintf(filename, "%04d_%s_XXXXXX", rank, str);
	fd=mkstemp(filename);
	if(fd==-1){
		fprintf(stderr, "%s::%d  Error opening %s for reading.\n", __FILE__, __LINE__, filename);
		return (FILE*)-1;
	}
	close(fd);
	f = fopen(filename, "w");
	return f;
}

