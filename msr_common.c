/* msr_common.c
 */
#include <stdio.h>
#include <unistd.h>	// sleep()
#include <sys/time.h>	// getttimeofday()
#include "msr_common.h"

//int msr_debug;

#ifdef TEST_HARNESS
#include "msr_core.h"
#include "msr_rapl.h"
#include "msr_pebs.h"
uint64_t global_test;
/*
static void
spin(uint64_t i){
	for(global_test=0; global_test<i; global_test++);
}
*/




static void
test_power_meters(){
	struct power_unit units;
	struct power_info info[NUM_DOMAINS];
	double joules[NUM_DOMAINS]; 
	uint64_t last_raw_joules[NUM_DOMAINS];
	struct timeval now;
#ifdef ARCH_062D
	int i;
#endif
	msr_debug=0;
	get_rapl_power_unit(0, &units);

#ifdef ARCH_062A
	get_power_info(0, PKG_DOMAIN, &info[PKG_DOMAIN],&units);
	get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units, 
			  &last_raw_joules[PKG_DOMAIN]);
	get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units,
			  &last_raw_joules[PP0_DOMAIN]);
	get_energy_status(0, PP1_DOMAIN, &joules[PP1_DOMAIN], &units,
			  &last_raw_joules[PP1_DOMAIN]);
	while(1){
		gettimeofday(&now, NULL);
		get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units,
				  &last_raw_joules[PKG_DOMAIN]);
		get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units,
				  &last_raw_joules[PP0_DOMAIN]);
		get_energy_status(0, PP1_DOMAIN, &joules[PP1_DOMAIN], &units,
				  &last_raw_joules[PP1_DOMAIN]);
		fprintf(stderr, "timestamp= %0lu.%0lu  pkg_J= %15.10lf  pp0_J= %15.10lf  pp1_J= %15.10lf\n", 
			now.tv_sec, now.tv_usec,
			joules[PKG_DOMAIN],joules[PP0_DOMAIN],joules[PP1_DOMAIN] );
		sleep(1);
	}
#endif

#ifdef ARCH_062D
	msr_debug=1;
	get_power_info(0, PKG_DOMAIN, 	&info[PKG_DOMAIN], 	&units);
	get_power_info(0, DRAM_DOMAIN, 	&info[DRAM_DOMAIN],	&units);
	msr_debug=0;
	get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units);
	get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units);
	get_energy_status(0, DRAM_DOMAIN, &joules[DRAM_DOMAIN], &units);
	for(i=0; i<100; i++){
		gettimeofday(&now, NULL);
		get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units);
		get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units);
		get_energy_status(0, DRAM_DOMAIN, &joules[DRAM_DOMAIN], &units);
		fprintf(stderr, "timestamp= %0lu.%0lu  pkg_J= %15.10lf  pp0_J= %15.10lf  dram_J= %15.10lf\n", 
			now.tv_sec, now.tv_usec,
			joules[PKG_DOMAIN],joules[PP0_DOMAIN],joules[DRAM_DOMAIN] );
		sleep(10);
	}
#endif

	return;
}

static void
test_pebs(){
	uint64_t counter[4], reset[4];
	counter[0] = MEM_LOAD_RETIRED__L2_LINE_MISS;
	counter[1] = UNUSED_COUNTER;
	counter[2] = UNUSED_COUNTER;
	counter[3] = UNUSED_COUNTER;
	reset[0] = 100;
	reset[1] = 0;
	reset[2] = 0;
	reset[3] = 0;
	pebs_init( 10, counter, reset );
	dump_pebs();
	return;
}



int
main(int argc, char **argv){
	msr_debug=1;
	init_msr();
	test_power_meters();
	if(0){
		test_pebs();
	}
	return 0;
}

#endif //TEST_HARNESS
