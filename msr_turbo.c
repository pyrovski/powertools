#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "msr_common.h"
#include "msr_core.h"
#include "msr_turbo.h"

// MSRs common to 062A and 062D.

// setting bit 38 high DISABLES turbo mode. (BIOS ONLY)
#define MSR_MISC_ENABLE			0x1A0	// aka IA32_MISC_ENABLE

#define MSR_IA32_PERF_CTL               0x00000199

void
enable_turbo(int socket){
	uint64_t val;
	read_msr( socket, MSR_IA32_PERF_CTL, &val );
	// Set bit 32 to 0.
	val &= ((uint64_t)-1) ^ ((uint64_t)1) << 32;
	write_msr( socket, MSR_IA32_PERF_CTL, val );
}


void
disable_turbo(int socket){
	uint64_t val;
	read_msr( socket, MSR_IA32_PERF_CTL, &val );
	// Set bit 32 to 1.
	val |= ((uint64_t)1) << 32;
	write_msr( socket, MSR_IA32_PERF_CTL, val );
}

