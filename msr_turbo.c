#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "msr_common.h"
#include "msr_core.h"
#include "msr_turbo.h"

// MSRs common to 062A and 062D.
#define MSR_MISC_ENABLE			0x1A0	// aka IA32_MISC_ENABLE
						// setting bit 38 high DISABLES turbo mode.


void
enable_turbo(int cpu){
	uint64_t val;
	read_msr( cpu, MSR_MISC_ENABLE, &val );
	// Set bit 38 to 0.
	val &= ((uint64_t)-1) ^ ((uint64_t)1) << 38;
	write_msr( cpu, MSR_MISC_ENABLE, val );
}


void
disable_turbo(int cpu){
	uint64_t val;
	read_msr( cpu, MSR_MISC_ENABLE, &val );
	// Set bit 38 to 1.
	val |= ((uint64_t)1) << 38;
	write_msr( cpu, MSR_MISC_ENABLE, val );
}

