#include <stdio.h>
#include <stdint.h>
#include "msr_rapl.h"
#include "msr_core.h"
#include "cpuid.h"

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
      
    }
  }
  
  finalize_msr();
  return 0;
}
