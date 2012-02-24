#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include "msr_rapl.h"
#include "msr_core.h"
#include "cpuid.h"

//#define _DEBUG


/*!
  enable/disable turbo for all cores
 */








void usage(const char * const argv0){
  fprintf(stderr, 
	  "Usage: %s [-e for enable] [-d for disable] [-P <package watts>] [-0 PP0 watts]\n", 
	  argv0);
}

int main(int argc, char ** argv){
  struct rapl_state_s rapl_state;
  FILE *f = fopen("/tmp/rapl_clamp", "w");
  rapl_init(&rapl_state, argc, argv, f, 1);
  parse_proc_cpuinfo();

  int enable = 1;

  /*
    get desired performance levels
   */
  int opt;
  float PKG_Watts = 0, PP0_Watts = 0;
  while((opt = getopt(argc, argv, "ed0:P:")) != -1){
    switch(opt){
    case 'e':
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
    default:
      usage(argv[0]);
    }
  }
  if((PP0_Watts != 0 && enable == 0) || 
     (PKG_Watts != 0 && enable == 0)){
    fprintf(stderr, 
	    "cannot simultaneously apply power bound and disable rapl\n");
    exit(1);
  }
  
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
      
    }else{
      // 
      struct power_limit_s power_limit = {
	.lock = 0
      };
      set_power_limit(i, PKG_DOMAIN, &power_limit);
    }
  }
  
  rapl_finalize(&rapl_state, 0);
  return 0;
}
