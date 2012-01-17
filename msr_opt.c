#include <unistd.h>	//getopt
#include <getopt.h>
#include <stdlib.h>	//strtoll
#include <stdint.h>
#include "msr_rapl.h"
#include "msr_core.h"

static struct option long_options[] = {
	{"MSR_PKG_POWER_LIMIT",		1, 0, MSR_PKG_POWER_LIMIT}, 	// RW
	{"MSR_PP0_POWER_LIMIT",		1, 0, MSR_PP0_POWER_LIMIT},	// RW
	{"MSR_DRAM_POWER_LIMIT",	1, 0, MSR_DRAM_POWER_LIMIT},	// RW
	{ 0,				0, 0, 0}
 };

static char *
msr2str( uint64_t msr ){
	switch(msr){
		case MSR_PKG_POWER_LIMIT:	return "MSR_PKG_POWER_LIMIT";	break;	
		case MSR_PP0_POWER_LIMIT:	return "MSR_PP0_POWER_LIMIT";	break;
		case MSR_DRAM_POWER_LIMIT:	return "MSR_DRAM_POWER_LIMIT";	break;
		default:			return "WTF?";			break;
	}
	return "Probably should never be seeing this.";
}

void
parse_opts( int argc, char **argv ){
	int option_index, c, cpu;
	char short_options[] = "";
	uint64_t msr_pkg_power_limit=-1, msr_pp0_power_limit=-1, msr_dram_power_limit=-1;
	char *env;

	// First, check the environment variables.
	env = getenv("MSR_PKG_POWER_LIMIT");
	if(env){
		msr_pkg_power_limit = strtoll( env, NULL, 0 );
	}

	env = getenv("MSR_PP0_POWER_LIMIT");
	if(env){
		msr_pp0_power_limit = strtoll( env, NULL, 0 );
	}

	env = getenv("MSR_DRAM_POWER_LIMIT");
	if(env){
		msr_dram_power_limit = strtoll( env, NULL, 0 );
	}

	// Override this with the command line.
	optind = 1;	// Reset the options index --- is this a nice thing to do?
	while(1){
		
		c = getopt_long_only(argc, argv, short_options, long_options, &option_index);
		fprintf(stderr, "%s::%d c=%d\n", __FILE__, __LINE__, c);
		if( c == -1 ){
			break;
		}
		switch(c){
			case MSR_PKG_POWER_LIMIT:	
				msr_pkg_power_limit = strtoll( optarg, NULL, 0 );
				break;
			case MSR_PP0_POWER_LIMIT:	
				msr_pp0_power_limit = strtoll( optarg, NULL, 0 );
				break;
			case MSR_DRAM_POWER_LIMIT:
				msr_dram_power_limit = strtoll( optarg, NULL, 0 );
				break;
			default:
				break;
		}
	}
	optind = 1;	// Set it back, just to be nice.
	
	// Now write the MSRs.  Zero is a valid value
	for( cpu=0; cpu<NUM_PACKAGES; cpu++ ){
		if( msr_pkg_power_limit != -1 ){
			fprintf(stderr, "%s::%d setting %s to 0x%lx on cpu %d\n", 
				__FILE__, __LINE__, msr2str(MSR_PKG_POWER_LIMIT), msr_pkg_power_limit, cpu);
			write_msr( cpu, c, msr_pkg_power_limit );
		}
		if( msr_pp0_power_limit != -1 ){
			fprintf(stderr, "%s::%d setting %s to 0x%lx on cpu %d\n", 
				__FILE__, __LINE__, msr2str(MSR_PP0_POWER_LIMIT), msr_pp0_power_limit, cpu);
			write_msr( cpu, c, msr_pp0_power_limit );
		}
		if( msr_dram_power_limit != -1 ){
			fprintf(stderr, "%s::%d setting %s to 0x%lx on cpu %d\n", 
				__FILE__, __LINE__, msr2str(MSR_DRAM_POWER_LIMIT), msr_dram_power_limit, cpu);
			write_msr( cpu, c, msr_dram_power_limit );
		}
	}
}


