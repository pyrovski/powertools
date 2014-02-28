/* msr_clocks.c
 *
 * MPERF, APERF and friends.
 */

#include "msr_core.h"
#include "msr_clocks.h"


#define MSR_IA32_MPERF 0x000000e7
#define MSR_IA32_APERF 0x000000e8

//!@todo convert to a single read
void read_aperf_mperf(int core, uint64_t *aperf, uint64_t *mperf){
	read_msr( core, MSR_IA32_MPERF, mperf );
	read_msr( core, MSR_IA32_APERF, aperf );
}

