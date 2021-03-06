/* msr_common.h
 *
 * Data structures used across multiple modules.
 */
#ifndef MSR_COMMON_H
#define MSR_COMMON_H

#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include "msr_rapl.h"

extern int msr_debug;

#define MASK_RANGE(m,n) ((((uint64_t)1<<((m)-(n)+1))-1)<<(n))	// m>=n
#define MASK_VAL(x,m,n) (((uint64_t)(x)&MASK_RANGE((m),(n)))>>(n))

#define UNIT_SCALE(x,y) ((x)/(double)(1<<(y)))
#define UNIT_DESCALE(x,y) ((x)*(double)(1<<(y)))

static inline void doubleToTimespec(double s, struct timespec *ts){
	ts->tv_sec = lround(s);
	ts->tv_nsec = fmod(s, 1.0) * 1.0e9;
}

static inline uint64_t rdtsc(void)
{
  // use cpuid instruction to serialize
  asm volatile ("xorl %%eax,%%eax \n cpuid"
		::: "%rax", "%rbx", "%rcx", "%rdx");
  // compiler should eliminate one code path
  if (sizeof(long) == sizeof(uint64_t)) {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)(hi) << 32) | lo;
  }
  else {
    uint64_t tsc;
    asm volatile("rdtsc" : "=A" (tsc));
    return tsc;
  }
}

static inline double tsc_delta(const uint64_t *now, const uint64_t *then, 
			       const double *tsc_rate){
  return (*now >= *then ? (*now - *then) : (*then - *now))/ *tsc_rate;
}

static inline double 
ts_delta(struct timeval *start, struct timeval *stop){
	return (stop->tv_sec + stop->tv_usec/1000000.0) - (start->tv_sec + start->tv_usec/1000000.0);
}

static inline double measure_tsc(){
  struct timeval t1, t2;
  gettimeofday(&t1, 0);
  uint64_t tsc1 = rdtsc();
  sleep(1);
  gettimeofday(&t2, 0);
  uint64_t tsc2 = rdtsc();
  return (tsc1 < tsc2 ? (tsc2 - tsc1) : (tsc1 - tsc2)) / ts_delta(&t1, &t2);
}

static inline 
double
convert_raw_joules_delta(const uint64_t *j1, 
			 const uint64_t *j2,
			 const struct power_unit_s *units){
  uint64_t delta_joules;

  // FIXME:  This will give a wrong answer if we've wrapped around multiple times.
  delta_joules = *j2 - *j1;
  if( *j2 < *j1){
    delta_joules += 0x100000000;
  }
  return UNIT_SCALE(delta_joules, units->energy);
}
#endif  // MSR_COMMON_H
