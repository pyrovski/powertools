#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "msr_common.h"
#include "msr_core.h"
#include "msr_rapl.h"
#include "msr_opt.h"
#include "blr_util.h"

double
joules2watts( double joules, struct timeval *start, struct timeval *stop ){

	return joules/
		(
			(stop->tv_usec - start->tv_usec)/(double)1000000.0
			+
			stop->tv_sec   - start->tv_sec
		);
}

#ifdef ARCH_SANDY_BRIDGE
void
get_rapl_power_unit(int cpu, struct power_unit *units){
	uint64_t val;
	read_msr( cpu, MSR_RAPL_POWER_UNIT, &val );
	//p->power  = (val & MASK_RANGE( 3, 0) );	// Taken from figure 14-16,
	//p->energy = (val & MASK_RANGE(12, 8) ); // page 14(29).
	//p->time	  = (val & MASK_RANGE(19,16) );
	units->power  	= MASK_VAL(val, 3, 0);
	units->energy	= MASK_VAL(val,12, 8);
	units->time  	= MASK_VAL(val,19,16);

	if(msr_debug){
		fprintf(stderr, "%s::%d  multipliers: p=0x%x e=0x%x t=0x%x\n", __FILE__, __LINE__, 	units->power, units->energy, units->time);
		fprintf(stderr, "%s::%d  One tick equals %15.10lf watts.\n", __FILE__, __LINE__, 	UNIT_SCALE(1, units->power));
		fprintf(stderr, "%s::%d  One tick equals %15.10lf joules.\n", __FILE__, __LINE__, 	UNIT_SCALE(1, units->energy));
		fprintf(stderr, "%s::%d  One tick equals %15.10lf seconds.\n", __FILE__, __LINE__, 	UNIT_SCALE(1, units->time));
	}
}

static const char *
domain2str( int domain ){
	switch(domain){
		case PKG_DOMAIN:	return "PKG";	break;
		case PP0_DOMAIN:	return "PP0";	break;
#ifdef ARCH_062A
		case PP1_DOMAIN:	return "PP1";	break;
#endif
#ifdef ARCH_062D
		case DRAM_DOMAIN:	return "DRAM";	break;
#endif
		default: 		assert(0);	break;
	}
}

void
get_raw_energy_status( int cpu, int domain, uint64_t *raw_joules ){
	switch(domain){
		case PKG_DOMAIN: 	read_msr( cpu, MSR_PKG_ENERGY_STATUS, raw_joules );	
					break;
		case PP0_DOMAIN: 	read_msr( cpu, MSR_PP0_ENERGY_STATUS, raw_joules );	
					break;
#ifdef ARCH_062A
		case PP1_DOMAIN: 	read_msr( cpu, MSR_PP1_ENERGY_STATUS, raw_joules );	
					break;
#endif
#ifdef ARCH_062D
		case DRAM_DOMAIN:	read_msr( cpu, MSR_DRAM_ENERGY_STATUS, raw_joules );		
				 	break;
#endif
		default: 		assert(0);						
					break;
	}
	if(msr_debug){
		fprintf(stderr, "%s::%d  raw joules (%s) = %lu\n", 
			__FILE__, __LINE__, domain2str(domain), *raw_joules);
	}
}

void
get_energy_status(int cpu, int domain, double *joules, struct power_unit *units ){
	// FIXME This does not handle wraparound.  Need to figure out what MaxJoules is.
	static uint64_t last_joules[NUM_PACKAGES][NUM_DOMAINS]; 
	uint64_t current_joules, delta_joules;
	get_raw_energy_status( cpu, domain, &current_joules );
	delta_joules = current_joules - last_joules[cpu][domain];	
	last_joules[cpu][domain] = current_joules;
	if(joules != NULL){
		*joules = UNIT_SCALE(delta_joules, units->energy);
		if(msr_debug){
			fprintf(stderr, "%s::%d  scaled delta joules (%s) = %lf\n", 
					__FILE__, __LINE__, domain2str(domain), *joules);
		}
	}
}

void
get_raw_power_info( int cpu, int domain, uint64_t *pval ){
	switch(domain){
		case PKG_DOMAIN:	read_msr( cpu, MSR_PKG_POWER_INFO, pval );		
					break;
#ifdef ARCH_062D
		case DRAM_DOMAIN:	read_msr( cpu, MSR_DRAM_POWER_INFO, pval );		
					break;
#endif
		default:		assert(0); // Not avail for PP0/PP1 domains.
					break;
	}
}

void
get_power_info( int cpu, int domain, struct power_info *info, struct power_unit *units ){
	uint64_t val;
	get_raw_power_info( cpu, domain, &val );
	info->max_time_window 		= MASK_VAL(val,53,48);
	info->max_power	   		= MASK_VAL(val,46,32);
	info->min_power	   		= MASK_VAL(val,30,16);
	info->thermal_spec_power 	= MASK_VAL(val,14, 0);

	info->max_time_window_sec	= UNIT_SCALE(info->max_time_window,   units->time);
	info->max_power_watts		= UNIT_SCALE(info->max_power,         units->power);
	info->min_power_watts		= UNIT_SCALE(info->min_power,         units->power);
	info->thermal_spec_power_watts	= UNIT_SCALE(info->thermal_spec_power,units->power);

	if(msr_debug){
		fprintf(stderr, "%s::%d (%s) Raw power info:  0x%lx\n",
				__FILE__, __LINE__, domain2str(domain), val);
		fprintf(stderr, "%s::%d (%s) max time window (%7lx) %10.5lf seconds.\n", 
				__FILE__, __LINE__, domain2str(domain), 
				info->max_time_window,
				info->max_time_window_sec);
		fprintf(stderr, "%s::%d (%s) max power       (%7lx) %10.5lf watts.\n", 
				__FILE__, __LINE__, domain2str(domain),
				info->max_power,
				info->max_power_watts);
		fprintf(stderr, "%s::%d (%s) min power       (%7lx) %10.5lf watts.\n", 
				__FILE__, __LINE__, domain2str(domain),
				info->min_power,
				info->min_power_watts);
		fprintf(stderr, "%s::%d (%s) thermal spec    (%7lx) %10.5lf watts.\n", 
				__FILE__, __LINE__, domain2str(domain),
				info->thermal_spec_power,
				info->thermal_spec_power_watts);
	}
}

void
get_raw_power_limit( int cpu, int domain, uint64_t *pval ){
	switch(domain){
		case PKG_DOMAIN: 	read_msr( cpu, MSR_PKG_POWER_LIMIT, pval );
					break;
		case PP0_DOMAIN: 	read_msr( cpu, MSR_PP0_POWER_LIMIT, pval );
					break;
#ifdef ARCH_062A				
		case PP1_DOMAIN: 	read_msr( cpu, MSR_PP1_POWER_LIMIT, pval );
					break;
#endif
#ifdef ARCH_062D				
		case DRAM_DOMAIN: 	read_msr( cpu, MSR_DRAM_POWER_LIMIT, pval );
					break;
#endif
		default:		assert(0);
					break;
	}
}

void
set_raw_power_limit( int cpu, int domain, uint64_t val ){
	switch(domain){
		case PKG_DOMAIN: 	write_msr( cpu, MSR_PKG_POWER_LIMIT, val );	
					break;
		case PP0_DOMAIN: 	write_msr( cpu, MSR_PP0_POWER_LIMIT, val );	
					break;
#ifdef ARCH_062A				
		case PP1_DOMAIN: 	write_msr( cpu, MSR_PP1_POWER_LIMIT, val );	
					break;
#endif
#ifdef ARCH_062D				
		case DRAM_DOMAIN: 	write_msr( cpu, MSR_DRAM_POWER_LIMIT, val );	
					break;
#endif
		default:		assert(0);					
					break;
	}
}

void
set_power_limit( int cpu, int domain, struct power_limit *limit ){
	uint64_t val = 0;
	if( domain == PKG_DOMAIN ){
		val |= (0x0001 & limit->lock) 		<< 63
		    |  (0x007F & limit->time_window_2) 	<< 49
		    |  (0x7FFF & limit->power_limit_2)	<< 32
		    |  (0x0001 & limit->clamp_2)	<< 48
		    |  (0x0001 & limit->enable_2)	<< 47;
	}else if(domain == DRAM_DOMAIN){
		val |= limit->lock		<< 32;
	}
	if( domain == PKG_DOMAIN || domain == DRAM_DOMAIN ){
		val |= (0x007F & limit->time_window_1)	<< 17
		    |  (0x7FFF & limit->power_limit_1)	<<  0
		    |  (0x0001 & limit->clamp_1)	<< 16
		    |  (0x0001 & limit->enable_1)	<< 15;
	}else{
		assert(0);
	}

	set_raw_power_limit( 0, domain, val );
}


void
get_power_limit( int cpu, int domain, struct power_limit *limit, struct power_unit *units ){
	uint64_t val;
	get_raw_power_limit( cpu, domain, &val );

	if(domain == PKG_DOMAIN){
		limit->time_window_2	= MASK_VAL(val, 53, 49);
		limit->power_limit_2	= MASK_VAL(val, 46, 32);
		limit->clamp_2		= MASK_VAL(val, 48, 48);
		limit->enable_2		= MASK_VAL(val, 47, 47);
		limit->time_multiplier_2= MASK_VAL(val, 55, 54);
		limit->lock		= MASK_VAL(val, 63, 63);
	}else{
		limit->lock		= MASK_VAL(val, 31, 31);
	}
	limit->power_limit_1	= MASK_VAL(val, 14,  0);
	limit->enable_1		= MASK_VAL(val, 15, 15);
	limit->clamp_1		= MASK_VAL(val, 16, 16);
	limit->time_window_1	= MASK_VAL(val, 21, 17);
	limit->time_multiplier_1= MASK_VAL(val, 23, 22);


	// There are several more clever ways of doing this.
	// I don't care.
	if(domain == PKG_DOMAIN){
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
		limit->time_window_2_sec	= UNIT_SCALE(limit->time_window_2, units->time) 
						  *
						  limit->time_multiplier_float_2;
		limit->power_limit_2_watts	= UNIT_SCALE(limit->power_limit_2, units->power);
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

	limit->time_window_1_sec	= UNIT_SCALE(limit->time_window_1, units->time) 
					  *
					  limit->time_multiplier_float_1;
	/*
	fprintf( stderr, "%s::%d limit->time_window_1= 0x%lx, units->time=0x%x, UNIT_SCALE(limit->time_window_1, units->time)=%lf, limit->time_window_1_sec=%lf, limit->time_multiplier_1=%lf\n",
			__FILE__, __LINE__, limit->time_window_1, units->time, UNIT_SCALE(limit->time_window_1, units->time), limit->time_window_1_sec, limit->time_multiplier_1);
			*/

	limit->power_limit_1_watts	= UNIT_SCALE(limit->power_limit_1, units->power);

	if( msr_debug && (domain == PKG_DOMAIN) ){

		fprintf( stderr, "%s::%d power limit time window 2 = 0x%lx\n", 
				__FILE__, __LINE__, limit->time_window_2 );
		fprintf( stderr, "%s::%d power limit power limit 2 = 0x%lx\n", 
				__FILE__, __LINE__, limit->power_limit_2 );
		fprintf( stderr, "%s::%d power limit clamp 2       = 0x%lx\n", 
				__FILE__, __LINE__, limit->clamp_2 );
		fprintf( stderr, "%s::%d power limit enable 2      = 0x%lx\n", 
				__FILE__, __LINE__, limit->enable_2 );
		fprintf( stderr, "%s::%d time multiplier 2         = 0x%lx\n", 
				__FILE__, __LINE__, limit->time_multiplier_2 );
		fprintf( stderr, "%s::%d time_window_2_sec         = %15.10lf\n", 
				__FILE__, __LINE__, limit->time_window_2_sec );
		fprintf( stderr, "%s::%d power_limit_2_watts        = %15.10lf\n", 
				__FILE__, __LINE__, limit->power_limit_2_watts );
		fprintf( stderr, "%s::%d time_multiplier_float_2    = %15.10lf\n", 
				__FILE__, __LINE__, limit->time_multiplier_float_2 );
	}
	if( msr_debug ){
		fprintf( stderr, "%s::%d power limit lock          = 0x%lx\n", 
				__FILE__, __LINE__, limit->lock );
		fprintf( stderr, "%s::%d power limit time window 1 = 0x%lx\n", 
				__FILE__, __LINE__, limit->time_window_1 );
		fprintf( stderr, "%s::%d power limit power limit 1 = 0x%lx\n", 
				__FILE__, __LINE__, limit->power_limit_1 );
		fprintf( stderr, "%s::%d power limit clamp 1       = 0x%lx\n", 
				__FILE__, __LINE__, limit->clamp_1 );
		fprintf( stderr, "%s::%d power limit enable 1      = 0x%lx\n", 
				__FILE__, __LINE__, limit->enable_1 );

		fprintf( stderr, "%s::%d time multiplier 1         = 0x%lx\n", 
				__FILE__, __LINE__, limit->time_multiplier_1 );
		
		fprintf( stderr, "%s::%d time_window_1_sec         = %15.10lf\n", 
				__FILE__, __LINE__, limit->time_window_1_sec );
		fprintf( stderr, "%s::%d power_limit_1_watts        = %15.10lf\n", 
				__FILE__, __LINE__, limit->power_limit_1_watts );
		fprintf( stderr, "%s::%d time_multiplier_float_1    = %15.10lf\n", 
				__FILE__, __LINE__, limit->time_multiplier_float_1 );
	}

}

void
get_raw_perf_status( int cpu, int domain, uint64_t *pstatus ){
	switch(domain){
#ifdef PKG_PERF_STATUS_AVAILABLE
		case PKG_DOMAIN: 	read_msr(cpu, MSR_PKG_PERF_STATUS, pstatus);
						break;
#endif
#ifdef ARCH_062D
		case DRAM_DOMAIN:	read_msr(cpu, MSR_DRAM_PERF_STATUS, pstatus);
						break;
#endif
		default:			assert(0);
						break;
	}

}

void
get_perf_status( int cpu, int domain, double *pstatus, struct power_unit *units ){
	uint64_t status;
	get_raw_perf_status( cpu, domain, &status );
	status = MASK_VAL(status, 31, 0);
	*pstatus = UNIT_SCALE( status, units->time );
	if(msr_debug){
		fprintf(stderr, "%s::%d (%s) status = 0x%lx (%15.10lf seconds)\n",
				__FILE__, __LINE__, domain2str(domain), status, *pstatus );
	}
}

void
get_raw_policy( int cpu, int domain, uint64_t *ppolicy ){
	switch( domain ){
		case PP0_DOMAIN:	read_msr( cpu, MSR_PP0_POLICY, ppolicy );	
					break;
#ifdef ARCH_062A
		case PP1_DOMAIN: 	read_msr( cpu, MSR_PP1_POLICY, ppolicy );	
					break;
#endif
		default:		assert(0);
					break;
	}
}

void
set_raw_policy( int cpu, int domain, uint64_t policy ){
	switch( domain ){
		case PP0_DOMAIN:	write_msr( cpu, MSR_PP0_POLICY, policy );	
					break;
#ifdef ARCH_062A
		case PP1_DOMAIN: 	write_msr( cpu, MSR_PP1_POLICY, policy );	
					break;
#endif
		default:		assert(0);
					break;
	}
}

void
get_policy( int cpu, int domain, uint64_t *ppolicy ){
	get_raw_policy( cpu, domain, ppolicy );
	if(msr_debug){
		fprintf(stderr, "%s::%d (%s) policy = 0x%lx (%lu)\n",
				__FILE__, __LINE__, domain2str(domain), *ppolicy, *ppolicy );
	}
}

void
set_policy( int cpu, int domain, uint64_t policy ){
	set_raw_policy( cpu, domain, policy );
}


struct rapl_state * 
rapl_init(int argc, char **argv, FILE *f){
	static struct rapl_state s;
	int cpu;
	init_msr();
	parse_opts( argc, argv );
	s.f = f;

	for(cpu=0; cpu<NUM_PACKAGES; cpu++){
		get_rapl_power_unit( cpu, &(s.power_unit[cpu]) );
		get_power_info(    cpu, PKG_DOMAIN,  &(s.power_info[cpu][PKG_DOMAIN]),          &(s.power_unit[cpu]) );
		get_power_info(    cpu, DRAM_DOMAIN, &(s.power_info[cpu][DRAM_DOMAIN]),         &(s.power_unit[cpu]) );
		
		get_power_limit(   cpu, PKG_DOMAIN,  &(s.power_limit[cpu][PKG_DOMAIN]),         &(s.power_unit[cpu]) );
		get_power_limit(   cpu, PP0_DOMAIN,  &(s.power_limit[cpu][PP0_DOMAIN]),         &(s.power_unit[cpu]) );
		get_power_limit(   cpu, DRAM_DOMAIN, &(s.power_limit[cpu][DRAM_DOMAIN]),        &(s.power_unit[cpu]) );

		//get_perf_status(   cpu, PKG_DOMAIN,  &(s.perf_status_start[cpu][PKG_DOMAIN]),   &(s.power_unit[cpu]) );
		//get_perf_status(   cpu, DRAM_DOMAIN, &(s.perf_status_start[cpu][PKG_DOMAIN]),   &(s.power_unit[cpu]) );

		get_energy_status( cpu, PKG_DOMAIN,  NULL, &(s.power_unit[cpu]) );
		get_energy_status( cpu, PP0_DOMAIN,  NULL, &(s.power_unit[cpu]) );
		get_energy_status( cpu, DRAM_DOMAIN, NULL, &(s.power_unit[cpu]) );

	}
	gettimeofday( &(s.start), NULL );
	return &s;
}

void
rapl_finalize( struct rapl_state *s ){

	int cpu;
	gettimeofday( &(s->finish), NULL );
	s->elapsed = ts_delta( &(s->start), &(s->finish) );
	for(cpu=0; cpu<NUM_PACKAGES; cpu++){
		//get_perf_status(   cpu, PKG_DOMAIN,  &(s->perf_status_finish[cpu][PKG_DOMAIN]),   &(s->power_unit[cpu]) );
		//get_perf_status(   cpu, DRAM_DOMAIN, &(s->perf_status_finish[cpu][PKG_DOMAIN]),   &(s->power_unit[cpu]) );

		get_energy_status( cpu, PKG_DOMAIN,  &(s->energy_status[cpu][PKG_DOMAIN]), &(s->power_unit[cpu]) );
		get_energy_status( cpu, PP0_DOMAIN,  &(s->energy_status[cpu][PP0_DOMAIN]), &(s->power_unit[cpu]) );
		get_energy_status( cpu, DRAM_DOMAIN, &(s->energy_status[cpu][DRAM_DOMAIN]),&(s->power_unit[cpu]) );
		s->avg_watts[cpu][PKG_DOMAIN] = joules2watts( s->energy_status[cpu][PKG_DOMAIN], &(s->start), &(s->finish) );
		s->avg_watts[cpu][PP0_DOMAIN] = joules2watts( s->energy_status[cpu][PP0_DOMAIN], &(s->start), &(s->finish) );
		s->avg_watts[cpu][DRAM_DOMAIN] = joules2watts( s->energy_status[cpu][DRAM_DOMAIN], &(s->start), &(s->finish) );

		// Rest all limits.
		write_msr( cpu, MSR_PKG_POWER_LIMIT, 0 );
		write_msr( cpu, MSR_PP0_POWER_LIMIT, 0 );
		write_msr( cpu, MSR_DRAM_POWER_LIMIT, 0 );
	}
	
	// Now the print statement from hell.
	
	//
	// Time 
	//
	fprintf(s->f, "%s ",
		"# elapsed");
	for(cpu=0; cpu<NUM_PACKAGES; cpu++){

		//
		// Avg Watts
		//
		fprintf(s->f, "%s_%d %s_%d %s_%d ",
				"PKG_Watts",		cpu,
			       	"PP0_Watts", 		cpu,
				"DRAM_Watts",		cpu);
				
		
		// 	
		// UNITS
		//

		fprintf( s->f, "%s_%d %s_%d %s_%d %s_%d %s_%d %s_%d ",
			"Units_Power_Bits",		cpu, 
			"Units_Energy_Bits",		cpu, 
			"Units_Time_Bits",		cpu, 
			"Units_Power_Watts",		cpu, 
			"Units_Energy_Joules",		cpu, 
			"Units_Time_Seconds",		cpu);


		//
		// LIMITS -- PKG Window 2
		//
		fprintf(s->f, "%s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d ",
			"PKG_Limit2_Enable",		cpu,
		       	"PKG_Limit2_Clamp",		cpu,
		       	"PKG_Limit2_Time_Bits",		cpu,
		       	"PKG_Limit2_Power_Bits",	cpu,
		       	"PKG_Limit2_Mult_Bits", 	cpu,
			"PKG_Limit2_Mult_Float",	cpu,
		       	"PKG_Limit2_Time_Seconds", 	cpu,
			"PKG_Limt2_Power_Watts", 	cpu); 
			
		//
		// LIMITS -- PKG Window 1
		//
		fprintf(s->f, "%s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d ",
			"PKG_Limit1_Enable",		cpu,
		       	"PKG_Limit1_Clamp",		cpu,
		       	"PKG_Limit1_Time_Bits",		cpu,
		       	"PKG_Limit1_Power_Bits",	cpu,
		       	"PKG_Limit1_Mult_Bits", 	cpu,
			"PKG_Limit1_Mult_Float",	cpu,
		       	"PKG_Limit1_Time_Seconds", 	cpu,
			"PKG_Limt1_Power_Watts", 	cpu); 
			
		//
		// LIMITS -- PP0 Window
		//
		fprintf(s->f, "%s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d ",
			"PP0_Limit1_Enable",		cpu,
		       	"PP0_Limit1_Clamp",		cpu,
		       	"PP0_Limit1_Time_Bits",		cpu,
		       	"PP0_Limit1_Power_Bits",	cpu,
		       	"PP0_Limit1_Mult_Bits", 	cpu,
			"PP0_Limit1_Mult_Float",	cpu,
		       	"PP0_Limit1_Time_Seconds", 	cpu,
			"PP0_Limt1_Power_Watts", 	cpu); 

		//
		// LIMITS -- DRAM Window
		//
		fprintf(s->f, "%s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d ",
			"DRAM_Limit1_Enable",		cpu,
		       	"DRAM_Limit1_Clamp",		cpu,
		       	"DRAM_Limit1_Time_Bits",		cpu,
		       	"DRAM_Limit1_Power_Bits",	cpu,
		       	"DRAM_Limit1_Mult_Bits", 	cpu,
			"DRAM_Limit1_Mult_Float",	cpu,
		       	"DRAM_Limit1_Time_Seconds", 	cpu,
			"DRAM_Limt1_Power_Watts", 	cpu); 
	}
			
	//
	// Done
	//
	fprintf(s->f, "\n");

	// Now the data on the following line....

	fprintf(s->f, "%lf ",
		s->elapsed);

	for(cpu=0; cpu<NUM_PACKAGES; cpu++){
			
		fprintf(s->f, "%lf %lf %lf ", 
			s->avg_watts[cpu][PKG_DOMAIN],
			s->avg_watts[cpu][PP0_DOMAIN],
			s->avg_watts[cpu][DRAM_DOMAIN]);

		fprintf( s->f, "%d %d %d %lf %lf %lf ", 
			s->power_unit[cpu].power, 
			s->power_unit[cpu].energy, 
			s->power_unit[cpu].time, 
			UNIT_SCALE(1, s->power_unit[cpu].power), 
			UNIT_SCALE(1, s->power_unit[cpu].energy), 
			UNIT_SCALE(1, s->power_unit[cpu].time));
		fprintf(s->f, "%lu %lu %lu %lu %lu %lf %lf %lf ", 
			s->power_limit[cpu][PKG_DOMAIN].clamp_2, 
			s->power_limit[cpu][PKG_DOMAIN].enable_2, 
			s->power_limit[cpu][PKG_DOMAIN].time_window_2, 
			s->power_limit[cpu][PKG_DOMAIN].power_limit_2, 
			s->power_limit[cpu][PKG_DOMAIN].time_multiplier_2, 
			s->power_limit[cpu][PKG_DOMAIN].time_multiplier_float_2, 
			s->power_limit[cpu][PKG_DOMAIN].time_window_2_sec, 
			s->power_limit[cpu][PKG_DOMAIN].power_limit_2_watts);

		fprintf(s->f, "%lu %lu %lu %lu %lu %lf %lf %lf ", 
			s->power_limit[cpu][PKG_DOMAIN].clamp_1, 
			s->power_limit[cpu][PKG_DOMAIN].enable_1, 
			s->power_limit[cpu][PKG_DOMAIN].time_window_1, 
			s->power_limit[cpu][PKG_DOMAIN].power_limit_1, 
			s->power_limit[cpu][PKG_DOMAIN].time_multiplier_1, 
			s->power_limit[cpu][PKG_DOMAIN].time_multiplier_float_1, 
			s->power_limit[cpu][PKG_DOMAIN].time_window_1_sec, 
			s->power_limit[cpu][PKG_DOMAIN].power_limit_1_watts);
				
		fprintf(s->f, "%lu %lu %lu %lu %lu %lf %lf %lf ", 
			s->power_limit[cpu][PP0_DOMAIN].clamp_1, 
			s->power_limit[cpu][PP0_DOMAIN].enable_1, 
			s->power_limit[cpu][PP0_DOMAIN].time_window_1, 
			s->power_limit[cpu][PP0_DOMAIN].power_limit_1, 
			s->power_limit[cpu][PP0_DOMAIN].time_multiplier_1, 
			s->power_limit[cpu][PP0_DOMAIN].time_multiplier_float_1, 
			s->power_limit[cpu][PP0_DOMAIN].time_window_1_sec, 
			s->power_limit[cpu][PP0_DOMAIN].power_limit_1_watts);

		fprintf(s->f, "%lu %lu %lu %lu %lu %lf %lf %lf ", 
			s->power_limit[cpu][DRAM_DOMAIN].clamp_1, 
			s->power_limit[cpu][DRAM_DOMAIN].enable_1, 
			s->power_limit[cpu][DRAM_DOMAIN].time_window_1, 
			s->power_limit[cpu][DRAM_DOMAIN].power_limit_1, 
			s->power_limit[cpu][DRAM_DOMAIN].time_multiplier_1, 
			s->power_limit[cpu][DRAM_DOMAIN].time_multiplier_float_1, 
			s->power_limit[cpu][DRAM_DOMAIN].time_window_1_sec, 
			s->power_limit[cpu][DRAM_DOMAIN].power_limit_1_watts);
				

	}
	fprintf(s->f, "\n");
	fclose(s->f);
}

#endif //ARCH_SANDY_BRIDGE


