#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "msr_common.h"
#include "blr_util.h"

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
void rapl_poll(const char * const filename, int log){
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
	    "pp1_J"
#endif
#ifdef ARCH_062D
	    "dram_J"
#endif
	    "\n");
    fprintf(logFile, "%lf\t%15.10lf\t%15.10lf"
	    "\t%15.10lf"
	    "\n", 
	    0.0, 0.0, 0.0
#ifdef ARCH_062A
	    ,0.0
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
	      "\t%15.10lf"
	      "\n", 
	      time,
	      joules[PKG_DOMAIN],
	      joules[PP0_DOMAIN]
#ifdef ARCH_062A
	      ,joules[PP1_DOMAIN]
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

