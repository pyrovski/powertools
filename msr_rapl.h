#ifndef MSR_ENUM_H
#define MSR_ENUM_H

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

#ifdef ARCH_SANDY_BRIDGE
struct power_units{
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
	double time_window_2_sec;
	double power_limit_2_watts;
	double time_window_1_sec;
	double power_limit_1_watts;
};

void get_power_units(int cpu, struct power_units *p);
void get_raw_joules(int cpu, uint64_t *raw_joules);
void get_joules(int cpu, struct power_units *p, double *joules);
void get_power_info(int cpu, struct power_info *info, struct power_units *units);
void get_raw_power_info( int cpu, uint64_t *pval );
void get_power_limit( int cpu, struct power_limit *limit, struct power_info *info );
void get_raw_power_limit( int cpu, uint64_t *pval );
#endif //ARCH_SANDY_BRIDGE

double get_power( double joules, struct timeval *start, struct timeval *stop );

#endif
