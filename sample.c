/* -*- Mode: C; tab-width: 2; c-basic-offset: 2 -*- */
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "msr_rapl.h"
#include "msr_common.h"

#define max(a,b) ((a)>(b)?(a):(b))

static int *extFlag = 0;

static void handler(int sig){
  if(msr_debug)
    printf("caught signal: %d (ALRM: %d, CHLD: %d)\n", 
					 sig, SIGALRM, SIGCHLD);
	if(sig == SIGCHLD){
		// stop sampling and return
		if(msr_debug)
			printf("Stopping sampling\n");
		*extFlag = 0;
	}
}

static sigset_t s;

void installSampleHandler(int *flag){
	extFlag = flag;
  sigemptyset(&s);
  sigaddset(&s, SIGALRM);
	sigaddset(&s, SIGCHLD);

  struct sigaction sa = {.sa_handler= &handler, 
			 .sa_mask = s, 
			 .sa_flags = 0, 
			 .sa_restorer = 0};
	sigdelset(&s, SIGCHLD);
	int status;
  status = sigaction(SIGCHLD, &sa, 0);
	if(status == -1)
		perror("SIGCHLD handler failed");
  status = sigaction(SIGALRM, &sa, 0);
	if(status == -1)
		perror("SIGALRM handler failed");
}

void msSample(const char * const filename, int log, double interval, 
							int numSockets, // starting from 0
							struct rapl_state_s *rapl_state
							)
{
	int socket;
	struct power_unit_s units;
  double joules[NUM_DOMAINS]; 
  uint64_t last_raw_joules[NUM_DOMAINS];
  struct timeval now;
  uint64_t tsc;
  double time = 0;
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
    fprintf(logFile, "timestamp\t");
		for(socket = 0; socket < numSockets; socket++)
			fprintf(logFile, "pkg_%1$d_J\tpp0_%1$d_J\t"
#ifdef ARCH_062A
							"pp1_%1$d_J\t",
#endif
#ifdef ARCH_062D
							"dram_%1$d_J\t",
#endif
							socket);
		fprintf(logFile, "\n%lf\t", 0);
		for(socket = 0; socket < numSockets; socket++)
			fprintf(logFile, "%15.10lf\t%15.10lf\t%15.10lf\t",
							0.0, 0.0, 0.0);
		fprintf(logFile, "\n");
  }
	
	int status;
  timer_t timerID;
  status = timer_create(CLOCK_MONOTONIC, 0, &timerID);
  struct itimerspec ts;
	doubleToTimespec(interval, &ts.it_interval);
	doubleToTimespec(interval, &ts.it_value);
	
  // synchronize with an update
  while(!joules[PKG_DOMAIN]){    
    usleep(10);
    get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units, 
		      &last_raw_joules[PKG_DOMAIN]);
  }
  
  status = timer_settime(timerID, 0, &ts, 0);

  gettimeofday(&now, NULL);
  tsc = rdtsc();

	for(socket = 0; socket < numSockets; socket++)
		get_all_energy_status(socket, rapl_state);
  
  double PKG_max_watts = 0, PP0_max_watts = 0;
  double PKG_total_joules = 0, PP0_total_joules = 0, delta;
  uint64_t lastPrint = 0, lastRead = tsc;
	siginfo_t sigInfo;

  while(extFlag && *extFlag){
    sigwaitinfo(&s, &sigInfo); // timer will wake us up
		
    tsc = rdtsc();
    double tscDelta = tsc_delta(&lastRead, &tsc, &tsc_rate);
		if(tscDelta < .0011)
			continue;

		for(socket = 0; socket < numSockets; socket++)
			get_all_energy_status(socket, rapl_state);

    //! @todo freq
    //read_aperf_mperf(0, &aperf, &mperf);

    lastRead = tsc;

    time += tscDelta;

    if(log){
			fprintf(logFile, "%lf\t", time);
			for(socket = 0; socket < numSockets; socket++)
				fprintf(logFile, "%15.10lf\t%15.10lf\t%15.10lf\t",
								rapl_state->energy_status[socket][PKG_DOMAIN],
								rapl_state->energy_status[socket][PP0_DOMAIN],
#ifdef ARCH_062A
								rapl_state->energy_status[socket][PP1_DOMAIN]
#endif
#ifdef ARCH_062D
								rapl_state->energy_status[socket][DRAM_DOMAIN]
#endif
								);
			fprintf(logFile, "\n");
    }
		//!@todo fix
    /* PKG_max_watts =  */
		/* 	max(PKG_max_watts, rapl_state->energy_status[socket][PKG_DOMAIN]/tscDelta); */
    /* PP0_max_watts =  */
		/* 	max(PP0_max_watts, rapl_state->energy_status[socket][PP0_DOMAIN]/tscDelta); */
    /* PKG_total_joules += rapl_state->energy_status[socket][PKG_DOMAIN]; */
    /* PP0_total_joules += rapl_state->energy_status[socket][PP0_DOMAIN]; */
    /* delta = tsc_delta(&lastPrint, &tsc, &tsc_rate); */
    /* if(delta > 1){ */
    /*   fprintf(stderr, "max interval power, average power in last second: " */
	  /*     "PKG: %10lf, %10lf, PP0: %10lf, %10lf\n",  */
	  /*     PKG_max_watts, PKG_total_joules / delta,  */
	  /*     PP0_max_watts, PP0_total_joules / delta); */
    /*   lastPrint = tsc; */
    /*   PKG_max_watts = 0; */
    /*   PP0_max_watts = 0; */
    /*   PKG_total_joules = 0; */
    /*   PP0_total_joules = 0; */
    /* } */
  } // while(1)
  
  //! @todo calculate average power
	if(msr_debug)
		printf("sampler finished\n");
  return;
}
