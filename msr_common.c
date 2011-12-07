/* msr_common.c
 */
#include <stdio.h>
#include <unistd.h>	// sleep()
#include <sys/time.h>	// getttimeofday()
#include "msr_common.h"

int msr_debug;

#ifdef TEST_HARNESS
#include "msr_core.h"
#include "msr_rapl.h"

uint64_t global_test;
/*
static void
spin(uint64_t i){
	for(global_test=0; global_test<i; global_test++);
}
*/



int
main(int argc, char **argv){
	struct power_units units;
	struct power_info info[NUM_DOMAINS];
	double joules[NUM_DOMAINS]; 
	struct timeval now;
	/*
	struct power_limit limit[NUM_DOMAINS];
	double power[4]; 
	uint64_t policy; 
	int i;
	struct timeval start, stop;
	*/
	msr_debug=0;
	init_msr();
	get_rapl_power_unit(0, &units);
	get_power_info(0, PKG_DOMAIN, &info[PKG_DOMAIN], &units);
	get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units);
	get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units);
	get_energy_status(0, PP1_DOMAIN, &joules[PP1_DOMAIN], &units);
	while(1){
		gettimeofday(&now, NULL);
		get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units);
		get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units);
		get_energy_status(0, PP1_DOMAIN, &joules[PP1_DOMAIN], &units);
		fprintf(stderr, "%0lu.%0lu pkg %15.10lf pp0 %15.10lf pp1 %15.10lf\n", 
			now.tv_sec, now.tv_usec,
			joules[PKG_DOMAIN],joules[PP0_DOMAIN],joules[PP1_DOMAIN] );
		sleep(1);
	}
/*	
	get_energy_status(0, PKG_DOMAIN, &joules, &units);
	gettimeofday( &start, NULL );
	sleep(1);
	gettimeofday( &stop,  NULL );
	get_energy_status(0, PKG_DOMAIN, &joules, &units);
	power = joules2watts( joules, &start, &stop );
	fprintf(stderr, "%s::%d %10.5lf sleep power\n", __FILE__, __LINE__, power);

	get_energy_status(0, PKG_DOMAIN, &joules, &units);
	gettimeofday( &start, NULL );
	for(i=0; i<1; i++){
		spin(1000000000LL);
	}
	gettimeofday( &stop,  NULL );
	get_energy_status(0, PKG_DOMAIN, &joules, &units);
	power = joules2watts( joules, &start, &stop );
	fprintf(stderr, "%s::%d %10.5lf single-core spin power\n", __FILE__, __LINE__, power);

	get_pkg_power_limit( 0, PKG_DOMAIN, &limit, &units );
	get_policy( 0, PP0_DOMAIN, &policy );
*/
	return 0;
}

#endif //TEST_HARNESS
