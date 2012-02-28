#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include "msr_rapl.h"
#include "msr_core.h"
#include "cpuid.h"
#include "msr_common.h"

//#define _DEBUG


void usage(const char * const argv0){
  fprintf(stderr, 
	  "Usage: %s [-e for enable] [-d for disable] [-P <package watts>] "
	  "[-0 PP0 watts] [-2 for 2nd PKG timing window] "
	  "[-w <window value (int)>] [-r to read values only]\n", 
	  argv0);
}

int main(int argc, char ** argv){
  struct rapl_state_s rapl_state;
  FILE *f = fopen("/tmp/rapl_clamp", "w");
  rapl_init(&rapl_state, argc, argv, f, 1);
  parse_proc_cpuinfo();

  int enable = 1;
  int enableSupplied = 0;
  int readOnly = 0;
  int opt;
  float PKG_Watts = 0, PP0_Watts = 0, PKG_Watts2 = 0;
  int windowSize = 0;
  int window2Size = 0;

  int w2Flag = 0;
  int p2Flag = 0;
  struct option options[] = {
    {"w2", 1, &w2Flag, 1},
    {"P2", 1, &p2Flag, 1},
    {0, 0, 0, 0}
  };
  int optionIndex = -1;
  
  /*
    get desired performance levels
   */
  while((opt = getopt_long(argc, argv, "edr0:P:w:", options, &optionIndex)) != -1){
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
    default:
      usage(argv[0]);
      exit(1);
    }
  }
  if((PP0_Watts != 0 && enable == 0) || 
     (PKG_Watts != 0 && enable == 0)){
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
    for (i = 0; i < config.sockets; i++){
      if(!enable){
#ifdef _DEBUG
	printf("disabling rapl clamping on socket %d (core %d)\n", 
	       i, config.map_socket_to_core[i][0]);
#endif
	write_msr( config.map_socket_to_core[i][0], MSR_PKG_POWER_LIMIT, 0 );
	write_msr( config.map_socket_to_core[i][0], MSR_PP0_POWER_LIMIT, 0 );
#ifdef ARCH_062D
	write_msr( config.map_socket_to_core[i][0], MSR_DRAM_POWER_LIMIT, 0 );
#endif
      }else{ // enable
	write_msr(i, MSR_PP0_POLICY, 31); // favor cores
#ifdef ARCH_062A
	write_msr(i, MSR_PP1_POLICY, 0); // over GPUs?
#endif

	if(!PKG_Watts && !PP0_Watts && enable){
	  fprintf(stderr, "no limits supplied\n");
	  exit(1);
	}
	// 
#ifdef _DEBUG
	printf("enabling rapl clamping on socket %d (core %d); PKG: %f, PP0: %f\n", 
	       i, config.map_socket_to_core[i][0],
	       PKG_Watts, PP0_Watts);
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
	  write_msr( config.map_socket_to_core[i][0], MSR_PKG_POWER_LIMIT, 0 );
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
	  write_msr( config.map_socket_to_core[i][0], MSR_PP0_POWER_LIMIT, 0 );
	}
      } // if(!enable) else
    } // for
  } // if(!readOnly)

  // don't reset MSRs on exit
  rapl_finalize(&rapl_state, 0);
  return 0;
}
