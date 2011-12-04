/* msr_common.c
 */

#include "msr_common.h"
int msr_debug;

#ifdef TEST_HARNESS
#include "msr_core.h"
#include "msr_rapl.h"

int
main(int argc, char **argv){
	struct power_units p;
	msr_debug=1;
	init_msr();
	get_power_units(0, &p);
	return 0;
}
#endif
