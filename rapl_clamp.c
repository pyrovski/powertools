#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "msr_rapl.h"
#include "msr_core.h"
#include "cpuid.h"
#include "msr_common.h"
#include "sample.h"

int sampling = 0;

void usage(const char * const argv0){
  fprintf(stderr, 
					"Usage: %s [-e for enable] [-d for disable] [-P <package watt limit>] "
#ifdef ARCH_062D
					"[-D <DRAM watt limit>] "
#endif
					"[--P2 <2nd PKG watt limit>] "
					"[--w2 <2nd PKG timing window>] "
					"[-0 <PP0 watt limit>] "
					"[-w <raw window value (int)>] "
					"[-r to read values only (/tmp/rapl_clamp by default)] "
					"[-o <summary output>] "
					"[-s [<sample interval in seconds>](default 1ms)]\n"
					"Limits will be applied to all sockets.\n", 
					argv0);
}

#define dlog(str) printf(str)

int main(int argc, char ** argv){
  struct rapl_state_s rapl_state;

  char filename[256], hostname[256], 
		sampleFilename[256] = "sample.dat";
  gethostname(hostname, 256);
  FILE *f = 0;
  snprintf(filename, 256, "rapl_clamp_%s", hostname);

  int enable = 1;
  int enableSupplied = 0;
  int readOnly = 0;
  int opt;
  float PKG_Watts = 0, PP0_Watts = 0, PKG_Watts2 = 0;
#ifdef ARCH_062D
  float DRAM_Watts = 0;
#endif
  int windowSize = 0;
  int window2Size = 0;
	double samplingInterval = .001;

  int w2Flag = 0;
  int p2Flag = 0;
  struct option options[] = {
    {"w2", 1, &w2Flag, 1},
    {"P2", 1, &p2Flag, 1},
    {0, 0, 0, 0}
  };
  int optionIndex = -1;
  
  /*!
    get desired performance levels
    @todo to run within rapl_clamp, use getopt "+" to supply args to execvp()
   */
  while((opt = getopt_long(argc, argv, 
			   "+hedr0:P:w:o:s::S:"
			   #ifdef ARCH_062D
			   "D:"
			   #endif
			   , options, &optionIndex)) != -1){
    switch(opt){
    case 0: // long options
      switch(optionIndex){
      case 0:
	window2Size = strtoul(optarg, 0, 0);
	break;
      case 1:
	PKG_Watts2 = strtoul(optarg, 0, 0);
	break;
      default:
	fprintf(stderr, 
		"invalid long option index: %d\n", optionIndex);
	exit(1);
      }
      break;
		case 'h':
			usage(argv[0]);
			return 0;
			break;
    case 'e':
      enableSupplied = 1;
      break;
    case 'd':
      enable = 0;
      break;
    case '0':
      // limit power for PP0
      PP0_Watts = strtof(optarg, 0);
      break;
    case 'P':
      PKG_Watts = strtof(optarg, 0);
      break;
    case 'w':
      windowSize = strtoul(optarg, 0, 0);
      break;
    case 'r':
      readOnly = 1;
      enable = 0;
      break;
#ifdef ARCH_062D
      DRAM_Watts = strtof(optarg, 0);
      break;
#endif
    case 'o':
      strncpy(filename, optarg, 256);
			filename[255] = 0;
      break;
		case 's':
			sampling = 1;
			if(optarg){
				samplingInterval = strtod(optarg, 0);
			}
			break;
		case 'S':
			strncpy(sampleFilename, optarg, 256);
			sampleFilename[255] = 0;
			break;
    default:
      usage(argv[0]);
      exit(1);
    }
  }

  filename[255] = 0;
  f = fopen(filename, "w");
  assert(f);
  rapl_init(&rapl_state, f, 1);
  int core, socket, local;
  get_cpuid(&mc_config, &mc_config_initialized, &core, &socket, &local);

  if((enable == 0) && 
     (PP0_Watts != 0 || PKG_Watts != 0 
#ifdef ARCH_062D
|| DRAM_Watts != 0
#endif
)){
    fprintf(stderr, 
	    "cannot simultaneously apply power bound and disable rapl\n");
    exit(1);
  }

  if(enableSupplied && readOnly){
    fprintf(stderr, 
	    "cannot enable rapl in read-only mode\n");
    exit(1);
  }

  if(w2Flag && !p2Flag){
    fprintf(stderr, 
	    "must also supply 2nd power limit if 2nd window given\n");
    exit(1);
  }

  
  if(!readOnly){
    // for now, apply the same limit to all sockets
    int i;
    for (i = 0; i < mc_config.sockets; i++){
      if(!enable){
#ifdef _DEBUG
	printf("disabling rapl clamping on socket %d (core %d)\n", 
	       i, mc_config.map_socket_to_core[i][0]);
#endif
	write_msr( mc_config.map_socket_to_core[i][0], MSR_PKG_POWER_LIMIT, 0 );
	write_msr( mc_config.map_socket_to_core[i][0], MSR_PP0_POWER_LIMIT, 0 );
#ifdef ARCH_062D
	write_msr( mc_config.map_socket_to_core[i][0], MSR_DRAM_POWER_LIMIT, 0 );
#endif
      }else{ // enable
	write_msr( mc_config.map_socket_to_core[i][0], MSR_PP0_POLICY, 31); // favor cores
#ifdef ARCH_062A
	write_msr( mc_config.map_socket_to_core[i][0], MSR_PP1_POLICY, 0); // over GPUs?
#endif

	if(enable && !PKG_Watts && !PP0_Watts
#ifdef ARCH_062D
	   && !DRAM_Watts
#endif
	   ){
	  fprintf(stderr, "no limits supplied\n");
		usage(argv[0]);
		exit(1);
	}
	// 
#ifdef _DEBUG
	printf("enabling rapl clamping on socket %d (core %d); PKG: %f, PP0: %f"
#ifdef ARCH_062D
	   ", DRAM: %f"
#endif
	       "\n", 
	       i, mc_config.map_socket_to_core[i][0],
	       PKG_Watts, PP0_Watts
#ifdef ARCH_062D
	       , DRAM_Watts
#endif
	       );
#endif
	if(PKG_Watts || PKG_Watts2){
	  struct power_limit_s power_limit1 = {
	    .lock = 0,
	    .clamp_1 = 1,
	    .enable_1 = 1,
	    .power_limit_1 = UNIT_DESCALE(PKG_Watts, rapl_state.power_unit[i].power),
	    .time_multiplier_1 = (windowSize >> 5) & 0b11,
	    .time_window_1 = windowSize & 0b11111,
	    .enable_2 = 0,
	    .clamp_2 = 0
	  };
	  struct power_limit_s power_limit2 = {
	    .lock = 0,
	    .clamp_2 = 1,
	    .enable_2 = 1,
	    .power_limit_2 = UNIT_DESCALE(PKG_Watts2, rapl_state.power_unit[i].power),
	    .time_multiplier_2 = (window2Size >> 5) & 0b11,
	    .time_window_2 = window2Size & 0b11111,
	    .enable_1 = 0,
	    .clamp_1 = 0
	  };
	  if(PKG_Watts && !PKG_Watts2)
	    set_power_limit(i, PKG_DOMAIN, &power_limit1);
	  else if(!PKG_Watts && PKG_Watts2)
	    set_power_limit(i, PKG_DOMAIN, &power_limit2);
	  else{
	    power_limit1.clamp_2 = power_limit2.clamp_2;
	    power_limit1.enable_2 = power_limit2.enable_2;
	    power_limit1.power_limit_2 = power_limit2.power_limit_2;
	    power_limit1.time_multiplier_2 = power_limit2.time_multiplier_2;
	    power_limit1.time_window_2 = power_limit2.time_window_2;
	    set_power_limit(i, PKG_DOMAIN, &power_limit1);
	  }
	} else { // !PKG_Watts && !PKG_Watts2
	  write_msr( mc_config.map_socket_to_core[i][0], MSR_PKG_POWER_LIMIT, 0 );
	}
	if(PP0_Watts){
	  struct power_limit_s power_limit = {
	    .lock = 0,
	    .clamp_1 = 1, // enable clamp
	    .enable_1 = 1, // enable limit
	    .power_limit_1 = UNIT_DESCALE(PP0_Watts, rapl_state.power_unit[i].power),
	    .time_multiplier_1 = (windowSize >> 5) & 0b11,
	    .time_window_1 = windowSize & 0b11111,
	    .enable_2 = 0, // don't use 2nd window
	    .clamp_2 = 0
	  };
	  set_power_limit(i, PP0_DOMAIN, &power_limit);
	} else { // !PP0_Watts
	  write_msr( mc_config.map_socket_to_core[i][0], MSR_PP0_POWER_LIMIT, 0 );
	}
#ifdef ARCH_062D
	if(DRAM_Watts){
	  struct power_limit_s power_limit = {
	    .lock = 0,
	    .clamp_1 = 1,
	    .enable_1 = 1,
	    .power_limit_1 = UNIT_DESCALE(DRAM_Watts, rapl_state.power_unit[i].power),
	    .time_multiplier_1 = (windowSize >> 5) & 0b11,
	    .time_window_1 = windowSize & 0b11111,
	    .enable_2 = 0,
	    .clamp_2 = 0
	  };
	  set_power_limit(i, DRAM_DOMAIN, &power_limit);
	} else {
	  write_msr(mc_config.map_socket_to_core[i][0], MSR_DRAM_POWER_LIMIT, 0);
	}
#endif
      } // if(!enable) else
    } // for
  } // if(!readOnly)

  if(optind < argc){
    int status;
    pid_t pid;
		if(sampling)
			installSampleHandler(&sampling);
    pid = fork();
    if(!pid){ // child
			printf("launching");
			int i;
			for(i = optind; i < argc; i++)
				printf(" %s", argv[i]);
			printf("\n");
      status = execvp(argv[optind], argv + optind);
      if(status){
				// complain
				perror("execvp failed");
				return -1;
      }
    } else if(pid == -1){
			perror("fork failed");
			return -1;
    } else { // parent
			if(sampling){
				msSample(sampleFilename, 1, samplingInterval);
			} else { // wait for program to quit
				wait(&status);
			}
    }
  }
	

  // don't reset MSRs on exit
  rapl_finalize(&rapl_state, 0);
  return 0;
}
