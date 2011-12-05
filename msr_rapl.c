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

double
get_power( double joules, struct timeval *start, struct timeval *stop ){

	return joules/
		(
			(stop->tv_usec - start->tv_usec)/(double)1000000.0
			+
			stop->tv_sec   - start->tv_sec
		);
}

#ifdef ARCH_SANDY_BRIDGE
void
get_power_units(int cpu, struct power_units *p){
	uint64_t val;
	read_msr( cpu, MSR_RAPL_POWER_UNIT, &val );
	//p->power  = (val & MASK_RANGE( 3, 0) );	// Taken from figure 14-16,
	//p->energy = (val & MASK_RANGE(12, 8) ); // page 14(29).
	//p->time	  = (val & MASK_RANGE(19,16) );
	p->power  = MASK_VAL(val, 3, 0);
	p->energy = MASK_VAL(val,12, 8);
	p->time	  = MASK_VAL(val,19,16);

	if(msr_debug){
		fprintf(stderr, "%s::%d (MSR_DBG) multipliers: p=%u e=%u t=%u\n",
				__FILE__, __LINE__, p->power, p->energy, p->time);
		fprintf(stderr, "%s::%d (3,0)=%lx, (12,8)=%lx, (19,16)=%lx\n",
				__FILE__, __LINE__, 
				MASK_RANGE(3,0),
				MASK_RANGE(12,8),
				MASK_RANGE(19,16));
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
	//*joules = delta_joules / ((double)(1<<(p->energy)));
	*joules = UNIT_SCALE(delta_joules,p->energy);
	if(msr_debug){
		fprintf(stderr, "%s::%d (MSR_DBG) scaled delta joules = %lf\n", 
				__FILE__, __LINE__, *joules);
	}
}

void
get_raw_power_info( int cpu, uint64_t *pval ){
	read_msr( cpu, MSR_PKG_POWER_INFO, pval );
}

void
get_power_info( int cpu, struct power_info *info, struct power_units *units ){
	uint64_t val;
	get_raw_power_info( cpu, &val );
	info->max_time_window 		= MASK_VAL(val,53,48);
	info->max_power	   		= MASK_VAL(val,46,32);
	info->min_power	   		= MASK_VAL(val,30,16);
	info->thermal_spec_power 	= MASK_VAL(val,14, 0);

	info->max_time_window_sec	= UNIT_SCALE(info->max_time_window,   units->time);
	info->max_power_watts		= UNIT_SCALE(info->max_power,         units->power);
	info->min_power_watts		= UNIT_SCALE(info->min_power,         units->power);
	info->thermal_spec_power_watts	= UNIT_SCALE(info->thermal_spec_power,units->power);

	if(msr_debug){
		fprintf(stderr, "%s::%d Raw power info:  %lx\n",
				__FILE__, __LINE__, val);
		fprintf(stderr, "%s::%d max time window (%7lx) %10.5lf seconds.\n", 
				__FILE__, __LINE__, 
				info->max_time_window,
				info->max_time_window_sec);
		fprintf(stderr, "%s::%d max power       (%7lx) %10.5lf watts.\n", 
				__FILE__, __LINE__, 
				info->max_power,
				info->max_power_watts);
		fprintf(stderr, "%s::%d min power       (%7lx) %10.5lf watts.\n", 
				__FILE__, __LINE__, 
				info->min_power,
				info->min_power_watts);
		fprintf(stderr, "%s::%d thermal spec    (%7lx) %10.5lf watts.\n", 
				__FILE__, __LINE__, 
				info->thermal_spec_power,
				info->thermal_spec_power_watts);
	}
}
#endif //ARCH_SANDY_BRIDGE
