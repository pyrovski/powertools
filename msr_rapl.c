#include <stdio.h>
#include <stdint.h>
#include "msr_common.h"
#include "msr_core.h"
#include "msr_rapl.h"

// MSRs common to 062A and 062D.
#ifdef ARCH_SANDY_BRIDGE
#define MSR_RAPL_POWER_UNIT		0x606	// (pkg) Section 14.7.1 "RAPL Interfaces"
#define MSR_PKG_POWER_LIMIT		0x610 	// Section 14.7.3 "Package RAPL Domain"
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
get_rapl_power_unit(int cpu, struct power_units *p){
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
		fprintf(stderr, "%s::%d (MSR_DBG) One tick equals %15.10lf watts.\n", 
				__FILE__, __LINE__, UNIT_SCALE(1, p->power) );
		fprintf(stderr, "%s::%d (MSR_DBG) One tick equals %15.10lf joules.\n", 
				__FILE__, __LINE__, UNIT_SCALE(1, p->energy) );
		fprintf(stderr, "%s::%d (MSR_DBG) One tick equals %15.10lf seconds.\n", 
				__FILE__, __LINE__, UNIT_SCALE(1, p->time) );
	}
}

void
get_raw_pkg_energy_status( int cpu, uint64_t *raw_joules ){
	read_msr( cpu, MSR_PKG_ENERGY_STATUS, raw_joules );
	if(msr_debug){
		fprintf(stderr, "%s::%d (MSR_DBG) raw joules = %lu\n", 
				__FILE__, __LINE__, *raw_joules);
	}
}

void
get_pkg_energy_status(int cpu,double *joules, struct power_units *units ){
	static uint64_t last_joules=0; 
	uint64_t current_joules, delta_joules;
	get_raw_pkg_energy_status( cpu, &current_joules );
	delta_joules = current_joules - last_joules;	
	last_joules = current_joules;
	*joules = UNIT_SCALE(delta_joules,units->energy);
	if(msr_debug){
		fprintf(stderr, "%s::%d (MSR_DBG) scaled delta joules = %lf\n", 
				__FILE__, __LINE__, *joules);
	}
}

void
get_raw_pkg_power_info( int cpu, uint64_t *pval ){
	read_msr( cpu, MSR_PKG_POWER_INFO, pval );
}

void
get_pkg_power_info( int cpu, struct power_info *info, struct power_units *units ){
	uint64_t val;
	get_raw_pkg_power_info( cpu, &val );
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

void
get_raw_pkg_power_limit( int cpu, uint64_t *pval ){
	read_msr( cpu, MSR_PKG_POWER_LIMIT, pval );
}

void
set_raw_pkg_power_limit( int cpu, uint64_t val ){
	write_msr( cpu, MSR_PKG_POWER_LIMIT, val );
}

void
set_pkg_power_limit( int cpu, struct power_limit *limit ){
	uint64_t val = 0;
	val = 
		  limit->lock 			<< 63
		| limit->time_window_2 		<< 49
		| limit->time_window_1		<< 17
		| limit->power_limit_2		<< 32
		| limit->power_limit_1		<<  0
		| limit->clamp_2		<< 48
		| limit->clamp_1		<< 16
		| limit->enable_2		<< 54
		| limit->enable_1		<< 15
		| limit->time_multiplier_2	<< 54
		| limit->time_multiplier_1	<< 22;

	set_raw_pkg_power_limit( 0, val );
}


void
get_pkg_power_limit( int cpu, struct power_limit *limit, struct power_units *units ){
	uint64_t val;
	get_raw_pkg_power_limit( cpu, &val );

	limit->lock		= MASK_VAL(val, 63, 63);
	limit->time_window_2	= MASK_VAL(val, 53, 49);
	limit->time_window_1	= MASK_VAL(val, 21, 17);
	limit->power_limit_2	= MASK_VAL(val, 46, 32);
	limit->power_limit_1	= MASK_VAL(val, 14,  0);
	limit->clamp_2		= MASK_VAL(val, 48, 48);
	limit->clamp_1		= MASK_VAL(val, 16, 16);
	limit->enable_2		= MASK_VAL(val, 47, 47);
	limit->enable_1		= MASK_VAL(val, 15, 15);
	limit->time_multiplier_2= MASK_VAL(val, 55, 54);
	limit->time_multiplier_1= MASK_VAL(val, 23, 22);


	// There are several more clever ways of doing this.
	// I don't care.
	switch(limit->time_multiplier_2){
		case 3:	limit->time_multiplier_float_2 = 1.3;	break;
		case 2:	limit->time_multiplier_float_2 = 1.2;	break;
		case 1:	limit->time_multiplier_float_2 = 1.1;	break;
		case 0:	limit->time_multiplier_float_2 = 1.0;	break;
		default:
			// Should never, ever get here.
			fprintf(stderr, "%s::%d BADNESS!  limit->time_multiplier_2 = %lx!\n",
					__FILE__, __LINE__, limit->time_multiplier_2 );
	}

	switch(limit->time_multiplier_1){
		case 3:	limit->time_multiplier_float_1 = 1.3;	break;
		case 2:	limit->time_multiplier_float_1 = 1.2;	break;
		case 1:	limit->time_multiplier_float_1 = 1.1;	break;
		case 0:	limit->time_multiplier_float_1 = 1.0;	break;
		default:
			// Should never, ever get here.
			fprintf(stderr, "%s::%d BADNESS!  limit->time_multiplier_1 = %lx!\n",
					__FILE__, __LINE__, limit->time_multiplier_1 );
	}

	limit->time_window_2_sec	= UNIT_SCALE(limit->time_window_2, units->time) 
					  *
					  limit->time_multiplier_2;

	limit->time_window_1_sec	= UNIT_SCALE(limit->time_window_1, units->time) 
					  *
					  limit->time_multiplier_1;

	limit->power_limit_2_watts	= UNIT_SCALE(limit->power_limit_2, units->power);
	limit->power_limit_1_watts	= UNIT_SCALE(limit->power_limit_1, units->power);


	if( msr_debug ){
		fprintf( stderr, "%s::%d power limit lock          = %lx\n", 
				__FILE__, __LINE__, limit->lock );
		fprintf( stderr, "%s::%d power limit time window 2 = %lx\n", 
				__FILE__, __LINE__, limit->time_window_2 );
		fprintf( stderr, "%s::%d power limit time window 1 = %lx\n", 
				__FILE__, __LINE__, limit->time_window_1 );
		fprintf( stderr, "%s::%d power limit power limit 1 = %lx\n", 
				__FILE__, __LINE__, limit->power_limit_2 );
		fprintf( stderr, "%s::%d power limit power limit 2 = %lx\n", 
				__FILE__, __LINE__, limit->power_limit_1 );
		fprintf( stderr, "%s::%d power limit clamp 2       = %lx\n", 
				__FILE__, __LINE__, limit->clamp_2 );
		fprintf( stderr, "%s::%d power limit clamp 1       = %lx\n", 
				__FILE__, __LINE__, limit->clamp_1 );
		fprintf( stderr, "%s::%d power limit enable 2      = %lx\n", 
				__FILE__, __LINE__, limit->enable_2 );
		fprintf( stderr, "%s::%d power limit enable 1      = %lx\n", 
				__FILE__, __LINE__, limit->enable_1 );

		fprintf( stderr, "%s::%d time multiplier 1         = %lx\n", 
				__FILE__, __LINE__, limit->time_multiplier_1 );
		fprintf( stderr, "%s::%d time multiplier 2         = %lx\n", 
				__FILE__, __LINE__, limit->time_multiplier_2 );
		
		fprintf( stderr, "%s::%d time_window_2_sec         = %15.10lf\n", 
				__FILE__, __LINE__, limit->time_window_2_sec );
		fprintf( stderr, "%s::%d time_window_1_sec         = %15.10lf\n", 
				__FILE__, __LINE__, limit->time_window_1_sec );
		fprintf( stderr, "%s::%d power_limit_2_watts        = %15.10lf\n", 
				__FILE__, __LINE__, limit->power_limit_2_watts );
		fprintf( stderr, "%s::%d power_limit_1_watts        = %15.10lf\n", 
				__FILE__, __LINE__, limit->power_limit_1_watts );
		fprintf( stderr, "%s::%d time_multiplier_float_1    = %15.10lf\n", 
				__FILE__, __LINE__, limit->time_multiplier_float_1 );
		fprintf( stderr, "%s::%d time_multiplier_float_2    = %15.10lf\n", 
				__FILE__, __LINE__, limit->time_multiplier_float_2 );
	}

}

void
get_raw_pkg_perf_status( int cpu, uint64_t *pstatus ){
	read_msr(cpu, MSR_PKG_PERF_STATUS, pstatus);
}

void
get_pkg_perf_status( int cpu, double *pstatus, struct power_units *units ){
	uint64_t status;
	get_raw_pkg_perf_status( cpu, &status );
	status = MASK_VAL(status, 31, 0);
	*pstatus = UNIT_SCALE( status, units->time );
	if(msr_debug){
		fprintf(stderr, "%s::%d status = 0x%lx (%15.10lf seconds)\n",
				__FILE__, __LINE__, status, *pstatus );
	}
}

#endif //ARCH_SANDY_BRIDGE
