#include <stdio.h>
#include <stdint.h>
#include "msr_turbo.h"
#include "msr_core.h"
#include "cpuid.h"

//#define _DEBUG

/*!
  enable/disable turbo for all cores
 */
int main(int argc, char ** argv){
  init_msr();

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
      enable_turbo(i);
    }else{
#ifdef _DEBUG
      printf("disabling turbo on socket %d (core %d)\n", 
	     i, config.map_socket_to_core[i][0]);
#endif
      disable_turbo(i);
    }
  }

  return 0;
}
