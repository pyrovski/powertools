#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "msr_common.h"
#include "msr_core.h"
#include "msr_rapl.h"
#include "msr_opt.h"
#include "blr_util.h"

static void print_rapl_state(struct rapl_state_s *s);
static void print_rapl_state_header(struct rapl_state_s *s);
static void get_all_status(int socket, struct rapl_state_s *s);

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
get_rapl_power_unit(int socket, struct power_unit *units){
	uint64_t val;
	read_msr( socket, MSR_RAPL_POWER_UNIT, &val );
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
get_raw_energy_status( int socket, int domain, uint64_t *raw_joules ){
	switch(domain){
		case PKG_DOMAIN: 	read_msr( socket, MSR_PKG_ENERGY_STATUS, raw_joules );	
					break;
		case PP0_DOMAIN: 	read_msr( socket, MSR_PP0_ENERGY_STATUS, raw_joules );	
					break;
#ifdef ARCH_062A
		case PP1_DOMAIN: 	read_msr( socket, MSR_PP1_ENERGY_STATUS, raw_joules );	
					break;
#endif
#ifdef ARCH_062D
		case DRAM_DOMAIN:	read_msr( socket, MSR_DRAM_ENERGY_STATUS, raw_joules );		
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
get_energy_status(int socket, int domain, double *joules, struct power_unit *units ){
	static uint64_t last_joules[NUM_PACKAGES][NUM_DOMAINS]; 
	uint64_t current_joules, delta_joules;
	get_raw_energy_status( socket, domain, &current_joules );
	// FIXME:  This will give a wrong answer if we've wrapped around multiple times.
	if( current_joules < last_joules[socket][domain]){
		current_joules += 0x100000000;
	}
	delta_joules = current_joules - last_joules[socket][domain];	
	last_joules[socket][domain] = current_joules;
	if(joules != NULL){
		*joules = UNIT_SCALE(delta_joules, units->energy);
		if(msr_debug){
			fprintf(stderr, "%s::%d  scaled delta joules (%s) = %lf\n", 
					__FILE__, __LINE__, domain2str(domain), *joules);
		}
	}
}

void
get_raw_power_info( int socket, int domain, uint64_t *pval ){
	switch(domain){
		case PKG_DOMAIN:	read_msr( socket, MSR_PKG_POWER_INFO, pval );		
					break;
#ifdef ARCH_062D
		case DRAM_DOMAIN:	read_msr( socket, MSR_DRAM_POWER_INFO, pval );		
					break;
#endif
		default:		assert(0); // Not avail for PP0/PP1 domains.
					break;
	}
}

void
get_power_info( int socket, int domain, struct power_info *info, struct power_unit *units ){
	uint64_t val;
	get_raw_power_info( socket, domain, &val );
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
get_raw_power_limit( int socket, int domain, uint64_t *pval ){
	switch(domain){
		case PKG_DOMAIN: 	read_msr( socket, MSR_PKG_POWER_LIMIT, pval );
					break;
		case PP0_DOMAIN: 	read_msr( socket, MSR_PP0_POWER_LIMIT, pval );
					break;
#ifdef ARCH_062A				
		case PP1_DOMAIN: 	read_msr( socket, MSR_PP1_POWER_LIMIT, pval );
					break;
#endif
#ifdef ARCH_062D				
		case DRAM_DOMAIN: 	read_msr( socket, MSR_DRAM_POWER_LIMIT, pval );
					break;
#endif
		default:		assert(0);
					break;
	}
}

void
set_raw_power_limit( int socket, int domain, uint64_t val ){
	switch(domain){
		case PKG_DOMAIN: 	write_msr( socket, MSR_PKG_POWER_LIMIT, val );	
					break;
		case PP0_DOMAIN: 	write_msr( socket, MSR_PP0_POWER_LIMIT, val );	
					break;
#ifdef ARCH_062A				
		case PP1_DOMAIN: 	write_msr( socket, MSR_PP1_POWER_LIMIT, val );	
					break;
#endif
#ifdef ARCH_062D				
		case DRAM_DOMAIN: 	write_msr( socket, MSR_DRAM_POWER_LIMIT, val );	
					break;
#endif
		default:		assert(0);					
					break;
	}
}

void
set_power_limit( int socket, int domain, struct power_limit *limit ){
	uint64_t val = 0;
	if( domain == PKG_DOMAIN ){
		val |= (0x0001 & limit->lock) 		<< 63
		    |  (0x007F & limit->time_window_2) 	<< 49
		    |  (0x7FFF & limit->power_limit_2)	<< 32
		    |  (0x0001 & limit->clamp_2)	<< 48
		    |  (0x0001 & limit->enable_2)	<< 47;
	}
#ifdef ARCH_062D
	else if(domain == DRAM_DOMAIN){
		val |= limit->lock		<< 32;
	}
#endif
	if( domain == PKG_DOMAIN
#ifdef ARCH_062D
	    || domain == DRAM_DOMAIN
#endif
	    ){
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
get_power_limit( int socket, int domain, struct power_limit *limit, struct power_unit *units ){
	uint64_t val;
	get_raw_power_limit( socket, domain, &val );

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
get_raw_perf_status( int socket, int domain, uint64_t *pstatus ){
	switch(domain){
#ifdef PKG_PERF_STATUS_AVAILABLE
		case PKG_DOMAIN: 	read_msr(socket, MSR_PKG_PERF_STATUS, pstatus);
						break;
#endif
#ifdef ARCH_062D
		case DRAM_DOMAIN:	read_msr(socket, MSR_DRAM_PERF_STATUS, pstatus);
						break;
#endif
		default:			assert(0);
						break;
	}

}

void
get_perf_status( int socket, int domain, double *pstatus, struct power_unit *units ){
	uint64_t status;
	get_raw_perf_status( socket, domain, &status );
	status = MASK_VAL(status, 31, 0);
	*pstatus = UNIT_SCALE( status, units->time );
	if(msr_debug){
		fprintf(stderr, "%s::%d (%s) status = 0x%lx (%15.10lf seconds)\n",
				__FILE__, __LINE__, domain2str(domain), status, *pstatus );
	}
}

void
get_raw_policy( int socket, int domain, uint64_t *ppolicy ){
	switch( domain ){
		case PP0_DOMAIN:	read_msr( socket, MSR_PP0_POLICY, ppolicy );	
					break;
#ifdef ARCH_062A
		case PP1_DOMAIN: 	read_msr( socket, MSR_PP1_POLICY, ppolicy );	
					break;
#endif
		default:		assert(0);
					break;
	}
}

void
set_raw_policy( int socket, int domain, uint64_t policy ){
	switch( domain ){
		case PP0_DOMAIN:	write_msr( socket, MSR_PP0_POLICY, policy );	
					break;
#ifdef ARCH_062A
		case PP1_DOMAIN: 	write_msr( socket, MSR_PP1_POLICY, policy );	
					break;
#endif
		default:		assert(0);
					break;
	}
}

void
get_policy( int socket, int domain, uint64_t *ppolicy ){
	get_raw_policy( socket, domain, ppolicy );
	if(msr_debug){
		fprintf(stderr, "%s::%d (%s) policy = 0x%lx (%lu)\n",
				__FILE__, __LINE__, domain2str(domain), *ppolicy, *ppolicy );
	}
}

void
set_policy( int socket, int domain, uint64_t policy ){
	set_raw_policy( socket, domain, policy );
}


struct rapl_state_s * 
rapl_init(int argc, char **argv, FILE *f){
	static struct rapl_state_s s;
	int socket;
	init_msr();
	parse_opts( argc, argv );
	fprintf(stderr, "%s::%d returned from parse_opts\n", __FILE__, __LINE__);
	s.f = f;

	for(socket=0; socket<NUM_PACKAGES; socket++){
		get_rapl_power_unit( socket, &(s.power_unit[socket]) );
		get_power_info(    socket, PKG_DOMAIN,  &(s.power_info[socket][PKG_DOMAIN]),          &(s.power_unit[socket]) );
#ifdef ARCH_062D
		get_power_info(    socket, DRAM_DOMAIN, &(s.power_info[socket][DRAM_DOMAIN]),         &(s.power_unit[socket]) );
#endif

		get_power_limit(   socket, PKG_DOMAIN,  &(s.power_limit[socket][PKG_DOMAIN]),         &(s.power_unit[socket]) );
		get_power_limit(   socket, PP0_DOMAIN,  &(s.power_limit[socket][PP0_DOMAIN]),         &(s.power_unit[socket]) );
#ifdef ARCH_062D
		get_power_limit(   socket, DRAM_DOMAIN, &(s.power_limit[socket][DRAM_DOMAIN]),        &(s.power_unit[socket]) );
#endif

		get_energy_status( socket, PKG_DOMAIN,  NULL, &(s.power_unit[socket]) );
		get_energy_status( socket, PP0_DOMAIN,  NULL, &(s.power_unit[socket]) );
#ifdef ARCH_062D
		get_energy_status( socket, DRAM_DOMAIN, NULL, &(s.power_unit[socket]) );
#endif
	}
	gettimeofday( &(s.start), NULL );
	return &s;
}

void
rapl_finalize( struct rapl_state_s *s ){

	int socket;
	gettimeofday( &(s->finish), NULL );
	s->elapsed = ts_delta( &(s->start), &(s->finish) );
	for(socket=0; socket<NUM_PACKAGES; socket++){
		get_all_status(socket, s);

		// Rest all limits.
		write_msr( socket, MSR_PKG_POWER_LIMIT, 0 );
		write_msr( socket, MSR_PP0_POWER_LIMIT, 0 );
#ifdef ARCH_062D
		write_msr( socket, MSR_DRAM_POWER_LIMIT, 0 );
#endif
	}

	// Now the print statement from hell.
	
	print_rapl_state_header(s);
	print_rapl_state(s);
	fclose(s->f);
}

static void get_all_status(int socket, struct rapl_state_s *s){
		get_energy_status( socket, PKG_DOMAIN,  &(s->energy_status[socket][PKG_DOMAIN]), &(s->power_unit[socket]) );
		get_energy_status( socket, PP0_DOMAIN,  &(s->energy_status[socket][PP0_DOMAIN]), &(s->power_unit[socket]) );
#ifdef ARCH_062D
		get_energy_status( socket, DRAM_DOMAIN, &(s->energy_status[socket][DRAM_DOMAIN]),&(s->power_unit[socket]) );
#endif
		s->avg_watts[socket][PKG_DOMAIN] = joules2watts( s->energy_status[socket][PKG_DOMAIN], &(s->start), &(s->finish) );
		s->avg_watts[socket][PP0_DOMAIN] = joules2watts( s->energy_status[socket][PP0_DOMAIN], &(s->start), &(s->finish) );
#ifdef ARCH_062D
		s->avg_watts[socket][DRAM_DOMAIN] = joules2watts( s->energy_status[socket][DRAM_DOMAIN], &(s->start), &(s->finish) );
#endif
}


static void print_rapl_state(struct rapl_state_s *s){
  
  int socket;
  
  // Now the data on the following line....

  fprintf(s->f, "%lf ",
	  s->elapsed);

  for(socket=0; socket<NUM_PACKAGES; socket++){
			
    fprintf(s->f, "%lf %lf ", 
	    s->avg_watts[socket][PKG_DOMAIN],
	    s->avg_watts[socket][PP0_DOMAIN]);
#ifdef ARCH_062D
    fprintf(s->f, "%lf ", s->avg_watts[socket][DRAM_DOMAIN]);
#endif

    fprintf(s->f, "%lf %ld %lf %ld %lf %ld %lf %ld ",
	    s->power_info[socket][PKG_DOMAIN].max_time_window_sec,
	    s->power_info[socket][PKG_DOMAIN].max_time_window,
	    s->power_info[socket][PKG_DOMAIN].max_power_watts,
	    s->power_info[socket][PKG_DOMAIN].max_power,
	    s->power_info[socket][PKG_DOMAIN].min_power_watts,
	    s->power_info[socket][PKG_DOMAIN].min_power,
	    s->power_info[socket][PKG_DOMAIN].thermal_spec_power_watts,
	    s->power_info[socket][PKG_DOMAIN].thermal_spec_power);


#ifdef ARCH_062D
    fprintf(s->f, "%lf %ld %lf %ld %lf %ld %lf %ld ",
	    s->power_info[socket][DRAM_DOMAIN].max_time_window_sec,
	    s->power_info[socket][DRAM_DOMAIN].max_time_window,
	    s->power_info[socket][DRAM_DOMAIN].max_power_watts,
	    s->power_info[socket][DRAM_DOMAIN].max_power,
	    s->power_info[socket][DRAM_DOMAIN].min_power_watts,
	    s->power_info[socket][DRAM_DOMAIN].min_power,
	    s->power_info[socket][DRAM_DOMAIN].thermal_spec_power_watts,
	    s->power_info[socket][DRAM_DOMAIN].thermal_spec_power);
#endif

    fprintf( s->f, "%d %d %d %lf %lf %lf ", 
	     s->power_unit[socket].power, 
	     s->power_unit[socket].energy, 
	     s->power_unit[socket].time, 
	     UNIT_SCALE(1, s->power_unit[socket].power), 
	     UNIT_SCALE(1, s->power_unit[socket].energy), 
	     UNIT_SCALE(1, s->power_unit[socket].time));
    fprintf(s->f, "%lu %lu %lu %lu %lu %lf %lf %lf ", 
	    s->power_limit[socket][PKG_DOMAIN].clamp_2, 
	    s->power_limit[socket][PKG_DOMAIN].enable_2, 
	    s->power_limit[socket][PKG_DOMAIN].time_window_2, 
	    s->power_limit[socket][PKG_DOMAIN].power_limit_2, 
	    s->power_limit[socket][PKG_DOMAIN].time_multiplier_2, 
	    s->power_limit[socket][PKG_DOMAIN].time_multiplier_float_2, 
	    s->power_limit[socket][PKG_DOMAIN].time_window_2_sec, 
	    s->power_limit[socket][PKG_DOMAIN].power_limit_2_watts);

    fprintf(s->f, "%lu %lu %lu %lu %lu %lf %lf %lf ", 
	    s->power_limit[socket][PKG_DOMAIN].clamp_1, 
	    s->power_limit[socket][PKG_DOMAIN].enable_1, 
	    s->power_limit[socket][PKG_DOMAIN].time_window_1, 
	    s->power_limit[socket][PKG_DOMAIN].power_limit_1, 
	    s->power_limit[socket][PKG_DOMAIN].time_multiplier_1, 
	    s->power_limit[socket][PKG_DOMAIN].time_multiplier_float_1, 
	    s->power_limit[socket][PKG_DOMAIN].time_window_1_sec, 
	    s->power_limit[socket][PKG_DOMAIN].power_limit_1_watts);
				
    fprintf(s->f, "%lu %lu %lu %lu %lu %lf %lf %lf ", 
	    s->power_limit[socket][PP0_DOMAIN].clamp_1, 
	    s->power_limit[socket][PP0_DOMAIN].enable_1, 
	    s->power_limit[socket][PP0_DOMAIN].time_window_1, 
	    s->power_limit[socket][PP0_DOMAIN].power_limit_1, 
	    s->power_limit[socket][PP0_DOMAIN].time_multiplier_1, 
	    s->power_limit[socket][PP0_DOMAIN].time_multiplier_float_1, 
	    s->power_limit[socket][PP0_DOMAIN].time_window_1_sec, 
	    s->power_limit[socket][PP0_DOMAIN].power_limit_1_watts);

#ifdef ARCH_062D
    fprintf(s->f, "%lu %lu %lu %lu %lu %lf %lf %lf ", 
	    s->power_limit[socket][DRAM_DOMAIN].clamp_1, 
	    s->power_limit[socket][DRAM_DOMAIN].enable_1, 
	    s->power_limit[socket][DRAM_DOMAIN].time_window_1, 
	    s->power_limit[socket][DRAM_DOMAIN].power_limit_1, 
	    s->power_limit[socket][DRAM_DOMAIN].time_multiplier_1, 
	    s->power_limit[socket][DRAM_DOMAIN].time_multiplier_float_1, 
	    s->power_limit[socket][DRAM_DOMAIN].time_window_1_sec, 
	    s->power_limit[socket][DRAM_DOMAIN].power_limit_1_watts);
#endif				

  }
  fprintf(s->f, "\n");

}

static void print_rapl_state_header(struct rapl_state_s *s){

  int socket;
  
  //
  // Time 
  //
  fprintf(s->f, "%s ",
	  "# elapsed");
  for(socket=0; socket<NUM_PACKAGES; socket++){

    //
    // Avg Watts
    //
    fprintf(s->f, "%s_%d %s_%d %s_%d ",
	    "PKG_Watts",		socket,
	    "PP0_Watts", 		socket,
	    "DRAM_Watts",		socket);
				
    //
    // INFO
    //

    fprintf(s->f, "%s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d ",
	    "PKG_Info_Max_Window_Sec",	socket,
	    "PKG_Info_Max_Window_Bits",	socket,
	    "PKG_Info_Max_Power_Watts", 	socket,
	    "PKG_Info_Max_Power_Bits", 	socket,
	    "PKG_Info_Min_Power_Watts",	socket,
	    "PKG_Info_Min_Power_Bits",	socket,
	    "PKG_Info_Thermal_Spec_Watts",	socket,
	    "PKG_Info_Thermal_Spec_Bits",	socket);


    fprintf(s->f, "%s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d ",
	    "DRAM_Info_Max_Window_Sec",	socket,
	    "DRAM_Info_Max_Window_Bits",	socket,
	    "DRAM_Info_Max_Power_Watts", 	socket,
	    "DRAM_Info_Max_Power_Bits", 	socket,
	    "DRAM_Info_Min_Power_Watts",	socket,
	    "DRAM_Info_Min_Power_Bits",	socket,
	    "DRAM_Info_Thermal_Spec_Watts",	socket,
	    "DRAM_Info_Thermal_Spec_Bits",	socket);



		
    // 	
    // UNITS
    //

    fprintf( s->f, "%s_%d %s_%d %s_%d %s_%d %s_%d %s_%d ",
	     "Units_Power_Bits",		socket, 
	     "Units_Energy_Bits",		socket, 
	     "Units_Time_Bits",		socket, 
	     "Units_Power_Watts",		socket, 
	     "Units_Energy_Joules",		socket, 
	     "Units_Time_Seconds",		socket);


    //
    // LIMITS -- PKG Window 2
    //
    fprintf(s->f, "%s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d ",
	    "PKG_Limit2_Enable",		socket,
	    "PKG_Limit2_Clamp",		socket,
	    "PKG_Limit2_Time_Bits",		socket,
	    "PKG_Limit2_Power_Bits",	socket,
	    "PKG_Limit2_Mult_Bits", 	socket,
	    "PKG_Limit2_Mult_Float",	socket,
	    "PKG_Limit2_Time_Seconds", 	socket,
	    "PKG_Limit2_Power_Watts", 	socket); 
			
    //
    // LIMITS -- PKG Window 1
    //
    fprintf(s->f, "%s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d ",
	    "PKG_Limit1_Enable",		socket,
	    "PKG_Limit1_Clamp",		socket,
	    "PKG_Limit1_Time_Bits",		socket,
	    "PKG_Limit1_Power_Bits",	socket,
	    "PKG_Limit1_Mult_Bits", 	socket,
	    "PKG_Limit1_Mult_Float",	socket,
	    "PKG_Limit1_Time_Seconds", 	socket,
	    "PKG_Limit1_Power_Watts", 	socket); 
			
    //
    // LIMITS -- PP0 Window
    //
    fprintf(s->f, "%s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d ",
	    "PP0_Limit1_Enable",		socket,
	    "PP0_Limit1_Clamp",		socket,
	    "PP0_Limit1_Time_Bits",		socket,
	    "PP0_Limit1_Power_Bits",	socket,
	    "PP0_Limit1_Mult_Bits", 	socket,
	    "PP0_Limit1_Mult_Float",	socket,
	    "PP0_Limit1_Time_Seconds", 	socket,
	    "PP0_Limit1_Power_Watts", 	socket); 

#ifdef ARCH_062D
    //
    // LIMITS -- DRAM Window
    //
    fprintf(s->f, "%s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d %s_%d ",
	    "DRAM_Limit1_Enable",		socket,
	    "DRAM_Limit1_Clamp",		socket,
	    "DRAM_Limit1_Time_Bits",	socket,
	    "DRAM_Limit1_Power_Bits",	socket,
	    "DRAM_Limit1_Mult_Bits", 	socket,
	    "DRAM_Limit1_Mult_Float",	socket,
	    "DRAM_Limit1_Time_Seconds", 	socket,
	    "DRAM_Limit1_Power_Watts", 	socket); 
#endif
  }
			
  //
  // Done
  //
  fprintf(s->f, "\n");
}
#endif //ARCH_SANDY_BRIDGE
