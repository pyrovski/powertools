#include <stdint.h>
#include <sys/time.h>
#include "msr_core.h"
#ifndef MSR_RAPL_H
#define MSR_RAPL_H
/* Power Units (bits 3:0):  Power related information 
 * (in Watts) is based on the multiplier, 1/2^{PU}; 
 * where PU is an unsigned integer represented by 
 * bits 3:0.  Default value is 0011b, indicating 
 * power unit is in 1/8 Watts increment.
 *
 * Energy Status Units (bits 12:8):  Energy related
 * information (in Joules) is based on the multiplier
 * 1/2^{ESU}; where ESU is an unsigned integer 
 * represented by bits 12:8.  Default value is 
 * 10000b, indicating energy status unit is in 
 * 15.3 microJoules increment.  
 * [1/2^{16} = 1.525879e-05].
 *
 * Time Units (bits 19:16):  Time realted information
 * (in seconds) is based on the multiplier, 1/2^{TU};
 * where TU is an unsigned integer represented by bits
 * 19:16.  Default value is 1010b, indicating time 
 * unit is 976 micro-seconds increment.
 *
 * [From Section 14.7.1 "RAPL Interfaces", v3B, p14(28).
 */

/* Availability
 *
 * Power Limit:		Running energy limits 		(RW)
 * Energy Status:	Accumulated Joules		( W)	
 * Policy:		Performance hint		(RW)
 * Perf Status:		Accumulated throttle time.	(R )
 * Power Info:		Min/Max domain power.		(R )
 *
 *
 * Domain	Power	Energy	Policy	Perf	Power
 * 		Limit	Status		Status	Info
 * --------------------------------------------------
 *  PKG		RW	R	n	n*	R
 *  DRAM	RW+	R+	n	R+	R+
 *  PP0		RW	R	RW	n	n
 *  PP1		RW-	R-	RW-	n	n
 *
 *  * "availability may be model specific."
 *  + ARCH_062D
 *  - ARCH_O62A
 */

// MSRs common to 062A and 062D.
#ifdef ARCH_SANDY_BRIDGE
#define MSR_RAPL_POWER_UNIT             0x606   // (pkg) Section 14.7.1 "RAPL Interfaces"
#define MSR_PKG_POWER_LIMIT             0x610   // Section 14.7.3 "Package RAPL Domain"
#define MSR_PKG_ENERGY_STATUS           0x611
#ifdef PKG_PERF_STATUS_AVAILABLE                                
#define MSR_PKG_PERF_STATUS             0x613   // aka MSR_PKG_RAPL_PERF_STATUS, uncertain availability.
#endif
#define MSR_PKG_POWER_INFO              0x614
#define MSR_PP0_POWER_LIMIT             0x638   // Section 14.7.4 "PP0/PP1 RAPL Domains"
#define MSR_PP0_ENERGY_STATUS           0x639
#define MSR_PP0_POLICY                  0x63A
#define MSR_PP0_PERF_STATUS             0x63B
#endif 

// Second-generation Core Sandy Bridge          // Section 14.7.4 "PP0/PP1 RAPL Domains"
#ifdef ARCH_062A                                
#define MSR_TURBO_RATIO_LIMIT           0x1AD   
#define MSR_PP1_POWER_LIMIT             0x640
#define MSR_PP1_ENERGY_STATUS           0x641
#define MSR_PP1_POLICY                  0x642
#endif

// Second-generation Xeon Sandy Bridge          // Section 14.7.5 "DRAM RAPL Domain"
#ifdef ARCH_062D
#define MSR_DRAM_POWER_LIMIT            0x618   
#define MSR_DRAM_ENERGY_STATUS          0x619
#define MSR_DRAM_PERF_STATUS            0x61B
#define MSR_DRAM_POWER_INFO             0x61C
#endif

#ifdef ARCH_SANDY_BRIDGE
struct power_unit{
	unsigned char time;
	unsigned char energy;
	unsigned char power;
};
struct power_info{
	uint64_t max_time_window;
	double   max_time_window_sec;
	uint64_t max_power;
	double   max_power_watts;
	uint64_t min_power;
	double   min_power_watts;
	uint64_t thermal_spec_power;
	double   thermal_spec_power_watts;
};

struct power_limit{
	uint64_t lock;
	uint64_t time_window_2;
	uint64_t clamp_2;
	uint64_t enable_2;
	uint64_t power_limit_2;
	uint64_t time_window_1;
	uint64_t clamp_1;
	uint64_t enable_1;
	uint64_t power_limit_1;
	uint64_t time_multiplier_1;
	uint64_t time_multiplier_2;
	double time_window_2_sec;
	double power_limit_2_watts;
	double time_window_1_sec;
	double power_limit_1_watts;
	double time_multiplier_float_1;
	double time_multiplier_float_2;
};

// get
void get_raw_energy_status(	int cpu, int domain, 	uint64_t *raw_joules);
void get_raw_pkg_power_limit( 	int cpu, 		uint64_t *pval      );		
void get_raw_power_info( 	int cpu, int domain, 	uint64_t *pval      );
void get_raw_perf_status( 	int cpu, int domain,	uint64_t *pstatus   );		
void get_raw_policy( 		int cpu, int domain, 	uint64_t *priority  );

void get_energy_status(		int cpu, int domain, 	double *joules, 		struct power_unit *units);
void get_power_limit( 		int cpu, int domain, 	struct power_limit *limit, 	struct power_unit *units);
void get_power_info(		int cpu, int domain, 	struct power_info *info, 	struct power_unit *units);
void get_perf_status(		int cpu, int domain, 	double *pstatus_sec, 		struct power_unit *units);	
void get_policy( 		int cpu, int domain, 	uint64_t *ppolicy 					 );

void get_rapl_power_unit(	int cpu, 		struct power_unit *p				         );


// set

void set_raw_power_limit( int cpu, int domain, uint64_t pval );			
void set_raw_policy( int cpu, int domain, uint64_t policy );

void set_power_limit( int cpu, int domain, struct power_limit *limit );		
void set_policy( int cpu, int domain, uint64_t policy );

enum{
	PKG_DOMAIN,
	PP0_DOMAIN,
#ifdef ARCH_062A
	PP1_DOMAIN,
#endif
#ifdef ARCH_062D
	DRAM_DOMAIN,
#endif
	NUM_DOMAINS
};


struct rapl_state{
	struct timeval start;
	struct timeval finish;
	double elapsed;
	double avg_watts[NUM_PACKAGES][NUM_DOMAINS];
	double energy_status[NUM_PACKAGES][NUM_DOMAINS];
	struct power_limit power_limit[NUM_PACKAGES][NUM_DOMAINS];
	struct power_unit  power_unit[NUM_PACKAGES];
	struct power_info  power_info[NUM_PACKAGES][NUM_DOMAINS];
	/*
	double perf_status_start[NUM_PACKAGES][NUM_DOMAINS];
	double perf_status_finish[NUM_PACKAGES][NUM_DOMAINS];
	uint64_t policy[NUM_PACKAGES][NUM_DOMAINS];
	*/
};



	

#endif //ARCH_SANDY_BRIDGE




double joules2watts( double joules, struct timeval *start, struct timeval *stop );

#endif //MSR_RAPL_H


