#include <stdlib.h>		// mkstmp, getenv
#include <unistd.h>		// close
#include "blr_util.h"

double 
ts_delta(struct timeval *start, struct timeval *stop){
	return (stop->tv_sec + stop->tv_usec/1000000.0) - (start->tv_sec + start->tv_usec/1000000.0);
}

FILE *
safe_mkstemp( const char *hostname, const char *tag, int mpi_rank ){
	FILE *f;
	int fd=-1;
	char filename[1024];
	char *benchmark;
	benchmark = getenv("BLR_BENCHMARK");
	sprintf(filename, "%s_%s_%s_%04d__XXXXXX", hostname, tag, benchmark, mpi_rank);
	//fprintf(stderr, "filename=%s\n", filename);
	fd=mkstemp(filename);
	if(fd==-1){
		fprintf(stderr, "%s::%d  Error opening %s for reading.\n", __FILE__, __LINE__, filename);
		return (FILE*)-1;
	}
	close(fd);
	f = fopen(filename, "w");
	return f;
}

void get_env_int(const char *name, int *val, int defaultVal){
	char *str;
	str = getenv(name);
	if(str)
	  *val = (int) strtol(str, (char **)NULL, 0);
	else
	  *val = defaultVal;
}

	
