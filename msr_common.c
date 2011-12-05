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

static void
spin(uint64_t i){
	for(global_test=0; global_test<i; global_test++);
}



int
main(int argc, char **argv){
	struct power_units units;
	struct power_info info;
	struct power_limit limit;
	double joules, power, perf_status;
	int i;
	struct timeval start, stop;
	msr_debug=1;
	init_msr();
	get_rapl_power_unit(0, &units);
	get_pkg_power_info(0, &info, &units);

	get_pkg_energy_status(0, &joules, &units);
	gettimeofday( &start, NULL );
	sleep(1);
	gettimeofday( &stop,  NULL );
	get_pkg_energy_status(0, &joules, &units);
	power = get_power( joules, &start, &stop );
	fprintf(stderr, "%s::%d %10.5lf sleep power\n", __FILE__, __LINE__, power);

	get_pkg_energy_status(0, &joules, &units);
	gettimeofday( &start, NULL );
	for(i=0; i<1; i++){
		spin(1000000000LL);
	}
	gettimeofday( &stop,  NULL );
	get_pkg_energy_status(0, &joules, &units);
	power = get_power( joules, &start, &stop );
	fprintf(stderr, "%s::%d %10.5lf single-core spin power\n", __FILE__, __LINE__, power);

	get_pkg_power_limit( 0, &limit, &units );
	get_pkg_perf_status( 0, &perf_status, &units);

	return 0;
}

#endif //TEST_HARNESS
