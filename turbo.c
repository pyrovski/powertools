#include <stdio.h>
#include <stdint.h>
#include "msr_turbo.h"
#include "msr_core.h"
#include "cpuid.h"
#include "msr_freq.h"

//#define _DEBUG

/*!
  enable/disable turbo for all cores
 */
int main(int argc, char ** argv){
  init_msr();
  parse_proc_cpuinfo();

  int enable = 1;
  if(argc > 1)
    enable = 0;

  int i;
  for (i = 0; i < config.sockets; i++){
    if(enable){
#ifdef _DEBUG
      printf("enabling turbo on socket %d (core %d)\n", 
	     i, config.map_socket_to_core[i][0]);
#endif
      enable_turbo(config.map_socket_to_core[i][0]);
    }else{
#ifdef _DEBUG
      printf("disabling turbo on socket %d (core %d)\n", 
	     i, config.map_socket_to_core[i][0]);
#endif
      disable_turbo(config.map_socket_to_core[i][0]);
    }
  }

  uint64_t aperf, mperf;
  for(i = 0; i < config.cores; i++){
    aperf = 0;
    mperf = 0;
    read_aperf_mperf(i, &aperf, &mperf);
  }

  return 0;
}
