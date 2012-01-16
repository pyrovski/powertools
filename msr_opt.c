#include <unistd.h>	//getopt
#include <getopt.h>
#include <stdlib.h>	//strtoll
#include <stdint.h>
#include "msr_rapl.h"
#include "msr_core.h"

static struct option long_options[] = {
	{"MSR_PKG_POWER_LIMIT",		1, 0, MSR_PKG_POWER_LIMIT}, 	// RW
	{"MSR_PP0_POWER_LIMIT",		1, 0, MSR_PP0_POWER_LIMIT},	// RW
	{"MSR_PP0_POLICY",		1, 0, MSR_PP0_POLICY},		// RW
	{"MSR_DRAM_POWER_LIMIT",	1, 0, MSR_DRAM_POWER_LIMIT},	// RW
	{ 0,				0, 0, 0}
 };

void
parse_opts( int argc, char **argv ){
	int option_index, c, i;
	char short_options[] = "";
	uint64_t val;
	while(1){
		
		c = getopt_long_only(argc, argv, short_options, long_options, &option_index);
		if( c == -1 ){
			break;
		}
		switch(c){
			case MSR_PKG_POWER_LIMIT:	//fallthrough
			case MSR_PP0_POWER_LIMIT:	//fallthrough
			case MSR_PP0_POLICY:		//fallthrough
			case MSR_DRAM_POWER_LIMIT:
				val = strtoll( optarg, NULL, 0 );
				for( i=0; i<NUM_PACKAGES; i++ ){
					write_msr( i, c, val );
				}
				break;	
			default:
				break;
		}
	}
}


