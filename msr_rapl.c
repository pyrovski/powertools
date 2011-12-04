#include <stdio.h>
#include <stdint.h>
#include "msr_common.h"
#include "msr_core.h"
#include "msr_rapl.h"

// MSRs common to 062A and 062D.
#ifdef ARCH_SANDY_BRIDGE
#define MSR_RAPL_POWER_UNIT		0x606	// (pkg) Section 14.7.1 "RAPL Interfaces"
#define MSR_PKG_RAPL_POWER_LIMIT	0x610 	// Section 14.7.3 "Package RAPL Domain"
#define MSR_PKG_ENERGY_STATUS		0x611
#define MSR_PKG_PERF_STATUS		0x613
#define MSR_PKG_POWER_INFO		0x614
#define MSR_PP0_POWER_LIMIT		0x638	// Section 14.7.4 "PP0/PP1 RAPL Domains"
#define MSR_PP0_ENERGY_STATUS		0x639
#define MSR_PP0_POLICY			0x63A
#define MSR_PP0_PERF_STATUS		0x63B
#endif 

// Second-generation Core Sandy Bridge 
#ifdef ARCH_062A				// Section 14.7.4 "PP0/PP1 RAPL Domains"
#define MSR_TURBO_RATIO_LIMIT		0x1AD	
#define MSR_PP1_POWER_LIMIT		0x640
#define MSR_PP1_ENERGY_STATUS		0x641
#define MSR_PP1_POLICY			0x642
#endif

// Second-generation Xeon Sandy Bridge
#ifdef ARCH_062D
#define MSR_DRAM_POWER_LIMIT		0x618	// Section 14.7.5 "DRAM RAPL Domain"
#define MSR_DRAM_ENERGY_STATUS		0x619
#define MSR_DRAM_PERF_STATUS		0x61B
#define MSR_DRAM_POWER_INFO		0x61C
#define MSR_
#endif

#ifdef ARCH_SANDY_BRIDGE
void
get_power_units(int cpu, struct power_units *p){
	uint64_t val;
	read_msr( cpu, MSR_RAPL_POWER_UNIT, &val );
	p->power  = (val & 0x00000f);
	p->energy = (val & 0x001f00) >>  8;
	p->time	  = (val & 0x0f0000) >> 16;

	if(msr_debug){
		fprintf(stderr, "%s::%d (MSR_DBG) multipliers: p=%u e=%u t=%u\n",
				__FILE__, __LINE__, p->power, p->energy, p->time);
	}
}

void
get_raw_joules( int cpu, uint64_t *raw_joules ){
	read_msr( cpu, MSR_PKG_ENERGY_STATUS, raw_joules );
	if(msr_debug){
		fprintf(stderr, "%s::%d (MSR_DBG) raw joules = %lu\n", 
				__FILE__, __LINE__, *raw_joules);
	}
}

void
get_joules(int cpu, struct power_units *p, double *joules){
	static uint64_t last_joules=0; 
	uint64_t current_joules, delta_joules;
	get_raw_joules( cpu, &current_joules );
	delta_joules = current_joules - last_joules;	
	last_joules = current_joules;
	*joules = delta_joules / ((double)(1<<(p->energy)));
	if(msr_debug){
		fprintf(stderr, "%s::%d (MSR_DBG) scaled delta joules = %lf\n", 
				__FILE__, __LINE__, *joules);
	}
}
#endif //ARCH_SANDY_BRIDGE
