/* msr_common.c
 */
#include <stdio.h>
#include <unistd.h>	// sleep()
#include <sys/time.h>	// getttimeofday()
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "msr_common.h"
#include "blr_util.h"

#ifdef TEST_HARNESS
#include "msr_core.h"
#include "msr_rapl.h"
#include "msr_pebs.h"
uint64_t global_test;
/*
static void
spin(uint64_t i){
	for(global_test=0; global_test<i; global_test++);
}
*/

static void handler(int sig){
  if(msr_debug)
    printf("caught signal: %d\n", sig);
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

static double measure_tsc(){
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

/*! a function to sample as fast as possible
  to determine underlying update pattern.

  From some sample data, it appears that PKG and PP0 power planes
  update from the same timebase, but with PKG and PP0 updates
  occurring independently, with PP0 updates occurring and average of
  1.292881e-06s after PKG updates, though the accuracy of this number
  may be affected by the efficiency of the measurement loop.

  PKG and PP0 power samples are highly correlated, as I am not using 
  the graphics portion of the chip.  This also indicates that 
  the consumer chips do not measure memory power in the PKG domain.
 */
static void poll(const char * const filename, int log){
  struct power_unit_s units;
  struct power_info_s info[NUM_DOMAINS];
  double joules[NUM_DOMAINS]; 
  uint64_t last_raw_joules[NUM_DOMAINS], last_raw_joules_tmp[NUM_DOMAINS];
  struct timeval now;
  uint64_t tsc, lastNonzero;
  double time = 0;


#ifdef ARCH_062D
  int i;
#endif

  double tsc_rate;
  FILE *rateFile = fopen("/tmp/tsc_rate", "r");
  //! @todo measure/read tsc rate
  if(!rateFile && errno == ENOENT){
    tsc_rate = measure_tsc();
    rateFile = fopen("/tmp/tsc_rate", "w");
    fprintf(rateFile, "%lf\n", tsc_rate);
  }else if(rateFile){
    // get rate from file
    fscanf(rateFile, "%lf", &tsc_rate);
  } else {
    perror("error opening /tmp/tsc_rate");
    exit(1);
  }
  fclose(rateFile);

  fprintf(stderr, "tsc rate: %lf\n", tsc_rate);

  FILE *logFile = 0;
  if(log){
    logFile = fopen(filename, "w");
    assert(logFile);
  }
  
  if(log){
    fprintf(logFile, "timestamp\tpkg_J\tpp0_J\t"
#ifdef ARCH_062A
	    //"pp1_J"
#endif
#ifdef ARCH_062D
	    "dram_J"
#endif
	    "\n");
    fprintf(logFile, "%lf\t%15.10lf\t%15.10lf"
	    //"\t%15.10lf"
	    "\n", 
	    0.0, 0.0, 0.0
#ifdef ARCH_062A
	    //,joules[PP1_DOMAIN]
#endif
#ifdef ARCH_062D
	    ,0.0
#endif
	    );
  }

  msr_debug=1;
  get_rapl_power_unit(0, &units);

  get_power_info(0, PKG_DOMAIN, &info[PKG_DOMAIN],&units);

  msr_debug = 0;

  get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units, 
		    &last_raw_joules[PKG_DOMAIN]);

  get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units, 
		    &last_raw_joules[PKG_DOMAIN]);
  // synchronize with an update
  while(!joules[PKG_DOMAIN]){    
    usleep(10);
    get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units, 
		      &last_raw_joules[PKG_DOMAIN]);
  }
  gettimeofday(&now, NULL);
  tsc = rdtsc();  
  
  get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units, 
		    &last_raw_joules[PKG_DOMAIN]);
  get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units,
		    &last_raw_joules[PP0_DOMAIN]);
#ifdef ARCH_062D
  get_power_info(0, DRAM_DOMAIN, 	&info[DRAM_DOMAIN],	&units);
#endif
#ifdef ARCH_062A
  get_energy_status(0, PP1_DOMAIN, &joules[PP1_DOMAIN], &units,
		    &last_raw_joules[PP1_DOMAIN]);
#endif

  lastNonzero = tsc;

  while(1){
    tsc = rdtsc();

    get_raw_energy_status(0, PKG_DOMAIN, &last_raw_joules_tmp[PKG_DOMAIN]);
    get_raw_energy_status(0, PP0_DOMAIN, &last_raw_joules_tmp[PP0_DOMAIN]);
#ifdef ARCH_062A
    get_raw_energy_status(0, PP1_DOMAIN, &last_raw_joules_tmp[PP1_DOMAIN]);
#endif
#ifdef ARCH_062D
    get_raw_energy_status(0, DRAM_DOMAIN, &last_raw_joules_tmp[DRAM_DOMAIN]);
#endif
    //! @todo freq
    //read_aperf_mperf(0, &aperf, &mperf);

    // wait for an update
    int PKG_updated = last_raw_joules_tmp[PKG_DOMAIN] != last_raw_joules[PKG_DOMAIN];
    int PP0_updated = last_raw_joules_tmp[PP0_DOMAIN] != last_raw_joules[PP0_DOMAIN];
#ifdef ARCH_062A
    int PP1_updated = last_raw_joules_tmp[PP1_DOMAIN] != last_raw_joules[PP1_DOMAIN];
#endif
#ifdef ARCH_062D
    int DRAM_updated = last_raw_joules_tmp[DRAM_DOMAIN] != last_raw_joules[DRAM_DOMAIN];
#endif
    if(!PKG_updated && !PP0_updated && 
#ifdef ARCH_062A
       !PP1_updated
#endif
#ifdef ARCH_062D
       !DRAM_updated
#endif
       ){
      continue;
    }

    double nzDelta = tsc_delta(&lastNonzero, &tsc, &tsc_rate);
    lastNonzero = tsc;
    time += nzDelta;

    // convert raw joules
    joules[PKG_DOMAIN] = 
      convert_raw_joules_delta(&last_raw_joules[PKG_DOMAIN], 
			       &last_raw_joules_tmp[PKG_DOMAIN], 
			       &units);
    joules[PP0_DOMAIN] = 
      convert_raw_joules_delta(&last_raw_joules[PP0_DOMAIN], 
			       &last_raw_joules_tmp[PP0_DOMAIN], 
			       &units);
#ifdef ARCH_062A
    joules[PP1_DOMAIN] = 
      convert_raw_joules_delta(&last_raw_joules[PP1_DOMAIN], 
			       &last_raw_joules_tmp[PP1_DOMAIN], 
			       &units);
#endif
#ifdef ARCH_062D
    joules[DRAM_DOMAIN] = 
      convert_raw_joules_delta(&last_raw_joules[DRAM_DOMAIN], 
			       &last_raw_joules_tmp[DRAM_DOMAIN], 
			       &units);
#endif

    last_raw_joules[PKG_DOMAIN] = last_raw_joules_tmp[PKG_DOMAIN];
    last_raw_joules[PP0_DOMAIN] = last_raw_joules_tmp[PP0_DOMAIN];
#ifdef ARCH_062A
    last_raw_joules[PP1_DOMAIN] = last_raw_joules_tmp[PP1_DOMAIN];
#endif
#ifdef ARCH_062D
    last_raw_joules[DRAM_DOMAIN] = last_raw_joules_tmp[DRAM_DOMAIN];
#endif
    // don't log the suspect readings
    // && joules[PKG_DOMAIN] < info[PKG_DOMAIN].thermal_spec_power_watts
    if(log){
      fprintf(logFile, "%lf\t%15.10lf\t%15.10lf"
	      //"\t%15.10lf"
	      "\n", 
	      time,
	      joules[PKG_DOMAIN],
	      joules[PP0_DOMAIN]
#ifdef ARCH_062A
	      //,joules[PP1_DOMAIN]
#endif
#ifdef ARCH_062D
	      ,joules[DRAM_DOMAIN]
#endif
	      );
    }
  } // while(1)
  
  //! @todo calculate average power
  
  return;
}

static void msSample(const char * const filename, int log){
  struct power_unit_s units;
  struct power_info_s info[NUM_DOMAINS];
  double joules[NUM_DOMAINS]; 
  uint64_t last_raw_joules[NUM_DOMAINS], last_raw_joules_tmp[NUM_DOMAINS];
  struct timeval now;
  uint64_t tsc;
  double time = 0;


#ifdef ARCH_062D
  int i;
#endif

  double tsc_rate;
  FILE *rateFile = fopen("/tmp/tsc_rate", "r");
  //! @todo measure/read tsc rate
  if(!rateFile && errno == ENOENT){
    tsc_rate = measure_tsc();
    rateFile = fopen("/tmp/tsc_rate", "w");
    fprintf(rateFile, "%lf\n", tsc_rate);
  }else if(rateFile){
    // get rate from file
    fscanf(rateFile, "%lf", &tsc_rate);
  } else {
    perror("error opening /tmp/tsc_rate");
    exit(1);
  }
  fclose(rateFile);

  fprintf(stderr, "tsc rate: %lf\n", tsc_rate);

  FILE *logFile = 0;
  if(log){
    logFile = fopen(filename, "w");
    assert(logFile);
  }
  
  if(log){
    fprintf(logFile, "timestamp\tpkg_J\tpp0_J\t"
#ifdef ARCH_062A
	    //"pp1_J"
#endif
#ifdef ARCH_062D
	    "dram_J"
#endif
	    "\n");
    fprintf(logFile, "%lf\t%15.10lf\t%15.10lf"
	    //"\t%15.10lf"
	    "\n", 
	    0.0, 0.0, 0.0
#ifdef ARCH_062A
	    //,joules[PP1_DOMAIN]
#endif
#ifdef ARCH_062D
	    ,0.0
#endif
	    );
  }

  sigset_t s;
  sigemptyset(&s);
  sigaddset(&s, SIGALRM);

  struct sigaction sa = {.sa_handler= &handler, 
			 .sa_mask = s, 
			 .sa_flags = 0, 
			 .sa_restorer = 0};
  int status = sigaction(SIGALRM, &sa, 0);

  timer_t timerID;
  status = timer_create(CLOCK_MONOTONIC, 0, &timerID);
  struct itimerspec ts = {{0, 100000}, // .1ms
			  {0, 100000}};

  msr_debug=1;
  get_rapl_power_unit(0, &units);

  get_power_info(0, PKG_DOMAIN, &info[PKG_DOMAIN],&units);

  msr_debug = 0;

  get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units, 
		    &last_raw_joules[PKG_DOMAIN]);

  get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units, 
		    &last_raw_joules[PKG_DOMAIN]);
  // synchronize with an update
  while(!joules[PKG_DOMAIN]){    
    usleep(10);
    get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units, 
		      &last_raw_joules[PKG_DOMAIN]);
  }
  gettimeofday(&now, NULL);
  tsc = rdtsc();  
  
  status = timer_settime(timerID, 0, &ts, 0);

  get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units, 
		    &last_raw_joules[PKG_DOMAIN]);
  get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units,
		    &last_raw_joules[PP0_DOMAIN]);
#ifdef ARCH_062D
  get_power_info(0, DRAM_DOMAIN, 	&info[DRAM_DOMAIN],	&units);
#endif
#ifdef ARCH_062A
  get_energy_status(0, PP1_DOMAIN, &joules[PP1_DOMAIN], &units,
		    &last_raw_joules[PP1_DOMAIN]);
#endif



  
  double PKG_max_watts = 0, PP0_max_watts = 0;
  double PKG_total_joules = 0, PP0_total_joules = 0, delta;
  uint64_t lastPrint = 0, lastNonzero = tsc;
  int glitch = 0;

  while(1){
    sigwaitinfo(&s, 0); // timer will wake us up
    tsc = rdtsc();

    get_raw_energy_status(0, PKG_DOMAIN, &last_raw_joules_tmp[PKG_DOMAIN]);
    get_raw_energy_status(0, PP0_DOMAIN, &last_raw_joules_tmp[PP0_DOMAIN]);
#ifdef ARCH_062A
    get_raw_energy_status(0, PP1_DOMAIN, &last_raw_joules_tmp[PP1_DOMAIN]);
#endif
#ifdef ARCH_062D
    get_raw_energy_status(0, DRAM_DOMAIN, &last_raw_joules_tmp[DRAM_DOMAIN]);
#endif
    //! @todo freq
    //read_aperf_mperf(0, &aperf, &mperf);

    // wait for an update
    //! @todo this needs fixing
    if(last_raw_joules_tmp[PKG_DOMAIN] == last_raw_joules[PKG_DOMAIN]){
      continue;
    }

    double nzDelta = tsc_delta(&lastNonzero, &tsc, &tsc_rate);
    if(nzDelta < .001){ // wait at least 1ms
      /*! @todo flag these in the log.
	Updates seem to come in two time bases, ~1 KHz and ~100 Hz.
	I'm guessing they correspond to distinct segments of the chip.
	If I sample frequently enough, I can separate the updates by frequency.	
       */
      if(!glitch){
	/*
	fprintf(logFile, "#%lf\t%lf\tglitch \n", 
		time + nzDelta,
		nzDelta
		);
	*/
	glitch = 1;
      }
      last_raw_joules_tmp[PKG_DOMAIN] = last_raw_joules[PKG_DOMAIN];
      continue;
    }
    glitch = 0;

    lastNonzero = tsc;

    // convert raw joules
    joules[PKG_DOMAIN] = 
      convert_raw_joules_delta(&last_raw_joules[PKG_DOMAIN], 
			       &last_raw_joules_tmp[PKG_DOMAIN], 
			       &units);
    joules[PP0_DOMAIN] = 
      convert_raw_joules_delta(&last_raw_joules[PP0_DOMAIN], 
			       &last_raw_joules_tmp[PP0_DOMAIN], 
			       &units);
#ifdef ARCH_062A
    joules[PP1_DOMAIN] = 
      convert_raw_joules_delta(&last_raw_joules[PP1_DOMAIN], 
			       &last_raw_joules_tmp[PP1_DOMAIN], 
			       &units);
#endif
#ifdef ARCH_062D
    joules[DRAM_DOMAIN] = 
      convert_raw_joules_delta(&last_raw_joules[DRAM_DOMAIN], 
			       &last_raw_joules_tmp[DRAM_DOMAIN], 
			       &units);
#endif

    last_raw_joules[PKG_DOMAIN] = last_raw_joules_tmp[PKG_DOMAIN];
    last_raw_joules[PP0_DOMAIN] = last_raw_joules_tmp[PP0_DOMAIN];
#ifdef ARCH_062A
    last_raw_joules[PP1_DOMAIN] = last_raw_joules_tmp[PP1_DOMAIN];
#endif
#ifdef ARCH_062D
    last_raw_joules[DRAM_DOMAIN] = last_raw_joules_tmp[DRAM_DOMAIN];
#endif

    time += nzDelta;

    // don't log the suspect readings
    // && joules[PKG_DOMAIN] < info[PKG_DOMAIN].thermal_spec_power_watts
    if(log){
      fprintf(logFile, "%lf\t%15.10lf\t%15.10lf"
	      //"\t%15.10lf"
	      "\n", 
	      time,
	      joules[PKG_DOMAIN],
	      joules[PP0_DOMAIN]
#ifdef ARCH_062A
	      //,joules[PP1_DOMAIN]
#endif
#ifdef ARCH_062D
	      ,joules[DRAM_DOMAIN]
#endif
	      );
    }
    PKG_max_watts = max(PKG_max_watts, joules[PKG_DOMAIN]/nzDelta);
    PP0_max_watts = max(PP0_max_watts, joules[PP0_DOMAIN]/nzDelta);
    PKG_total_joules += joules[PKG_DOMAIN];
    PP0_total_joules += joules[PP0_DOMAIN];
    delta = tsc_delta(&lastPrint, &tsc, &tsc_rate);
    if(delta > 1){
      fprintf(stderr, "max 1ms-power, average power in last second: "
	      "PKG: %10lf, %10lf, PP0: %10lf, %10lf\n", 
	      PKG_max_watts, PKG_total_joules / delta, 
	      PP0_max_watts, PP0_total_joules / delta);
      lastPrint = tsc;
      PKG_max_watts = 0;
      PP0_max_watts = 0;
      PKG_total_joules = 0;
      PP0_total_joules = 0;
    }
  } // while(1)
  
  //! @todo calculate average power
  
  return;
}

static void
test_power_meters(){
	struct power_unit_s units;
	struct power_info_s info[NUM_DOMAINS];
	double joules[NUM_DOMAINS]; 
	uint64_t last_raw_joules[NUM_DOMAINS];
	struct timeval now;
#ifdef ARCH_062D
	int i;
#endif
	msr_debug=1;
	get_rapl_power_unit(0, &units);

#ifdef ARCH_062A
	get_power_info(0, PKG_DOMAIN, &info[PKG_DOMAIN],&units);
	get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units, 
			  &last_raw_joules[PKG_DOMAIN]);
	get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units,
			  &last_raw_joules[PP0_DOMAIN]);
	get_energy_status(0, PP1_DOMAIN, &joules[PP1_DOMAIN], &units,
			  &last_raw_joules[PP1_DOMAIN]);
	while(1){
		gettimeofday(&now, NULL);
		get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units,
				  &last_raw_joules[PKG_DOMAIN]);
		get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units,
				  &last_raw_joules[PP0_DOMAIN]);
		get_energy_status(0, PP1_DOMAIN, &joules[PP1_DOMAIN], &units,
				  &last_raw_joules[PP1_DOMAIN]);
		fprintf(stderr, "timestamp= %0ld.%.6ld  pkg_J= %15.10lf  pp0_J= %15.10lf  pp1_J= %15.10lf\n", 
			now.tv_sec, now.tv_usec,
			joules[PKG_DOMAIN],joules[PP0_DOMAIN],joules[PP1_DOMAIN] );
		sleep(1);
	}
#endif

#ifdef ARCH_062D
	msr_debug=1;
	get_power_info(0, PKG_DOMAIN, 	&info[PKG_DOMAIN], 	&units);
	get_power_info(0, DRAM_DOMAIN, 	&info[DRAM_DOMAIN],	&units);
	msr_debug=0;
	get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units,
			  &last_raw_joules[PKG_DOMAIN]);
	get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units,
			  &last_raw_joules[PP0_DOMAIN]);
	get_energy_status(0, DRAM_DOMAIN, &joules[DRAM_DOMAIN], &units,
			  &last_raw_joules[DRAM_DOMAIN]);
	for(i=0; i<100; i++){
		gettimeofday(&now, NULL);
		get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units,
				  &last_raw_joules[DRAM_DOMAIN]);
		get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units,
			  &last_raw_joules[DRAM_DOMAIN]);
		get_energy_status(0, DRAM_DOMAIN, &joules[DRAM_DOMAIN], &units,
			  &last_raw_joules[DRAM_DOMAIN]);
		fprintf(stderr, "timestamp= %0ld.%.6ld  pkg_J= %15.10lf  pp0_J= %15.10lf  dram_J= %15.10lf\n", 
			now.tv_sec, now.tv_usec,
			joules[PKG_DOMAIN],joules[PP0_DOMAIN],joules[DRAM_DOMAIN] );
		sleep(10);
	}
#endif

	return;
}

static void
test_pebs(){
	uint64_t counter[4], reset[4];
	counter[0] = MEM_LOAD_RETIRED__L2_LINE_MISS;
	counter[1] = UNUSED_COUNTER;
	counter[2] = UNUSED_COUNTER;
	counter[3] = UNUSED_COUNTER;
	reset[0] = 100;
	reset[1] = 0;
	reset[2] = 0;
	reset[3] = 0;
	pebs_init( 10, counter, reset );
	dump_pebs();
	return;
}



int
main(int argc, char **argv){
	msr_debug=1;
	init_msr();
	if(argc > 1)
	  msSample(argv[1], 1);
	//poll(argv[1], 1);
	test_power_meters();
	if(0){
	  test_pebs();
	}
	return 0;
}

#endif //TEST_HARNESS
