#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "msr_common.h"
#include "msr_core.h"
#include "msr_turbo.h"
#include "cpuid.h"

// MSRs common to 062A and 062D.

// setting bit 38 high DISABLES turbo mode. (BIOS ONLY)
#define MSR_IA32_MISC_ENABLE            0x000001a0

#define MSR_IA32_PERF_CTL               0x00000199

//!@todo do we toggle all cores?

void
enable_turbo(int socket){
	int core = config.map_socket_to_core[socket][0];

	uint64_t val;
	/*
	read_msr( core, MSR_IA32_MISC_ENABLE, &val );
	// Set bit 38 to 0.
	val &= ((uint64_t)-1) ^ ((uint64_t)1) << 38;
	write_msr( core, MSR_IA32_MISC_ENABLE, val );
	*/

	read_msr( core, MSR_IA32_PERF_CTL, &val );
	// Set bit 32 to 0.
	val &= ((uint64_t)-1) ^ ((uint64_t)1) << 32;
	write_msr( core, MSR_IA32_PERF_CTL, val );
}


void
disable_turbo(int socket){
	int core = config.map_socket_to_core[socket][0];

	uint64_t val;
	/*
	read_msr( core, MSR_IA32_MISC_ENABLE, &val );
	// Set bit 38 to 0.
	val &= ((uint64_t)-1) ^ ((uint64_t)1) << 38;
	write_msr( core, MSR_IA32_MISC_ENABLE, val );
	*/

	read_msr( core, MSR_IA32_PERF_CTL, &val );
	// Set bit 32 to 1.
	val |= ((uint64_t)1) << 32;
	write_msr( core, MSR_IA32_PERF_CTL, val );
}

