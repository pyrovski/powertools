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
	struct power_units units;
	struct power_info info;
	double joules;
	msr_debug=1;
	init_msr();
	get_power_units(0, &units);
	get_power_info(0, &info, &units);
	get_joules(0, &units, &joules);
	sleep(10);
	get_joules(0, &units, &joules);
	return 0;
}
#endif
