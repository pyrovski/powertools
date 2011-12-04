/* msr_common.c
 */

#include <unistd.h>	//sleep()
#include "msr_common.h"
int msr_debug;

#ifdef TEST_HARNESS
#include "msr_core.h"
#include "msr_rapl.h"

int
main(int argc, char **argv){
	struct power_units p;
	uint64_t raw_joules;
	double joules;
	msr_debug=1;
	init_msr();
	get_power_units(0, &p);
	get_raw_joules(0, &raw_joules);
	sleep(1);
	get_raw_joules(0, &raw_joules);
	sleep(1);
	get_raw_joules(0, &raw_joules);
	sleep(1);
	get_joules(0, &p, &joules);
	sleep(1);
	get_joules(0, &p, &joules);
	sleep(1);
	get_joules(0, &p, &joules);
	return 0;
}
#endif
