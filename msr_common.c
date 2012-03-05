/* msr_common.c
 */
#include <stdio.h>
#include <unistd.h>	// sleep()
#include <sys/time.h>	// getttimeofday()
#include <time.h>
#include <signal.h>
#include <assert.h>
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

static void msSample(const char * const filename, int log){
  struct power_unit_s units;
  struct power_info_s info[NUM_DOMAINS];
  double joules[NUM_DOMAINS]; 
  uint64_t last_raw_joules[NUM_DOMAINS];
  struct timeval now;
  gettimeofday(&now, NULL);


#ifdef ARCH_062D
  int i;
#endif

  FILE *file = 0;
  if(log){
    file = fopen(filename, "w");
    assert(file);
  }
  
  msr_debug=1;
  get_rapl_power_unit(0, &units);

  get_power_info(0, PKG_DOMAIN, &info[PKG_DOMAIN],&units);
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

    fprintf(file, "timestamp\tpkg_J\tpp0_J\t"
#ifdef ARCH_062A
	    //"pp1_J"
#endif
#ifdef ARCH_062D
	    "dram_J"
#endif
	    "\n");
    

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
  status = timer_settime(timerID, 0, &ts, 0);

  msr_debug = 0;

  if(log){
    fprintf(file, "%0ld.%.6ld\t%15.10lf\t%15.10lf"
	    //"\t%15.10lf"
	    "\n", 
	    now.tv_sec, 
	    now.tv_usec,
	    0.0,
	    0.0
#ifdef ARCH_062A
	    //,joules[PP1_DOMAIN]
#endif
#ifdef ARCH_062D
	    ,0.0
#endif
	    );
  }

  
  double PKG_max_watts = 0, PP0_max_watts = 0;
  double PKG_total_joules, PP0_total_joules, delta;
  struct timeval lastPrint = {0,0}, lastNonzero = now;

  while(1){
    gettimeofday(&now, NULL);
    get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units,
		      &last_raw_joules[PKG_DOMAIN]);
    get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units,
		      &last_raw_joules[PP0_DOMAIN]);
#ifdef ARCH_062A
    get_energy_status(0, PP1_DOMAIN, &joules[PP1_DOMAIN], &units,
		      &last_raw_joules[PP1_DOMAIN]);
#endif
#ifdef ARCH_062D
    get_energy_status(0, DRAM_DOMAIN, &joules[DRAM_DOMAIN], &units,
		      &last_raw_joules[DRAM_DOMAIN]);
#endif
    //! @todo freq
    //read_aperf_mperf(0, &aperf, &mperf);

    if(!joules[PKG_DOMAIN])
      continue;

    if(log){
      fprintf(file, "%0ld.%.6ld\t%15.10lf\t%15.10lf"
	      //"\t%15.10lf"
	      "\n", 
	      now.tv_sec, 
	      now.tv_usec,
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
    double nzDelta = ts_delta(&lastNonzero, &now);
    PKG_max_watts = max(PKG_max_watts, joules[PKG_DOMAIN]/nzDelta);
    PP0_max_watts = max(PP0_max_watts, joules[PP0_DOMAIN]/nzDelta);
    lastNonzero = now;
    PKG_total_joules += joules[PKG_DOMAIN];
    PP0_total_joules += joules[PP0_DOMAIN];
    delta = ts_delta(&lastPrint, &now);
    if(delta > 1){
      fprintf(stderr, "max 1ms-power, average power in last second: "
	      "PKG: %10lf, %10lf, PP0: %10lf, %10lf\n", 
	      PKG_max_watts, PKG_total_joules / delta, 
	      PP0_max_watts, PP0_total_joules / delta);
      lastPrint = now;
      PKG_max_watts = 0;
      PP0_max_watts = 0;
      PKG_total_joules = 0;
      PP0_total_joules = 0;
    }
    sigwaitinfo(&s, 0); // timer will wake us up
  }
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
	test_power_meters();
	if(0){
	  test_pebs();
	}
	return 0;
}

#endif //TEST_HARNESS
