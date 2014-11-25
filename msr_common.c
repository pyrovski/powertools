/* msr_common.c
 */
#include <stdio.h>
#include <unistd.h>	// sleep()
#include <sys/time.h>	// getttimeofday()
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "msr_common.h"
#include "blr_util.h"
#include "sample.h"

#ifdef TEST_HARNESS
#include "msr_core.h"
#include "msr_rapl.h"
uint64_t global_test;

mcsup_nodeconfig_t mc_config;
int mc_config_initialized = 0;

static void
test_power_meters(){
	struct power_unit_s units;
	struct power_info_s info[NUM_DOMAINS];
	double joules[NUM_DOMAINS]; 
	uint64_t last_raw_joules[NUM_DOMAINS];
	struct timeval now;
	int i;
	msr_debug=1;
	get_rapl_power_unit(0, &units);

	for(i = 0; i < NUM_DOMAINS; i++)
		get_raw_energy_status( 0, i, last_raw_joules+i );
#ifdef ARCH_062A
	get_power_info(0, PKG_DOMAIN, &info[PKG_DOMAIN],&units);
	while(1){
		gettimeofday(&now, NULL);
		get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units,
				  &last_raw_joules[PKG_DOMAIN]);
		get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units,
				  &last_raw_joules[PP0_DOMAIN]);
		get_energy_status(0, PP1_DOMAIN, &joules[PP1_DOMAIN], &units,
				  &last_raw_joules[PP1_DOMAIN]);
		fprintf(stderr, "timestamp= %0ld.%.6ld  pkg_J= %15.10lf  pp0_J= %15.10lf  pp1_J= %15.10lf\n", 
			now.tv_sec, now.tv_usec,
			joules[PKG_DOMAIN],joules[PP0_DOMAIN],joules[PP1_DOMAIN] );
		sleep(1);
	}
#endif

#ifdef ARCH_062D
	msr_debug=1;
	get_power_info(0, PKG_DOMAIN, 	&info[PKG_DOMAIN], 	&units);
	get_power_info(0, DRAM_DOMAIN, 	&info[DRAM_DOMAIN],	&units);
	msr_debug=1;
	for(i=0; i<100; i++){
		gettimeofday(&now, NULL);
		get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units,
				  &last_raw_joules[PKG_DOMAIN]);
		get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units,
			  &last_raw_joules[PP0_DOMAIN]);
		get_energy_status(0, DRAM_DOMAIN, &joules[DRAM_DOMAIN], &units,
			  &last_raw_joules[DRAM_DOMAIN]);
		fprintf(stderr, "timestamp= %0ld.%.6ld  pkg_J= %15.10lf  pp0_J= %15.10lf  dram_J= %15.10lf\n", 
			now.tv_sec, now.tv_usec,
			joules[PKG_DOMAIN],joules[PP0_DOMAIN],joules[DRAM_DOMAIN] );
		sleep(10);
	}
#endif

	return;
}

extern void rapl_poll(const char * const filename, int log);

void usage(const char *name){
  printf("%s [-p <poll output>] [-s <sample output>]"
	 " [-m (test power meters)] [-b (test pebs)]\n", name);
}

int
main(int argc, char **argv){
	msr_debug=1;
	init_msr();

	struct rapl_state_s rapl_state;
  rapl_init(&rapl_state, 0, 0);

	int status;
	while((status = getopt(argc, argv, "p:s:m")) != -1){
	  switch(status){
	  case 'p':
	    rapl_poll(optarg, 1);
	    return 0;
	    break;
	  case 's':
	    msSample(optarg, 1, .001, 1, &rapl_state);
	    return 0;
	    break;
	  case 'm':
	    test_power_meters();
	    return 0;
	    break;
	  default:
	    usage(argv[0]);
	    exit(1);
	    break;
	  }
	}
	
	usage(argv[0]);
	return 0;
}

#endif //TEST_HARNESS
