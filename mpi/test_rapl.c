#include <mpi.h>
#include "../cpuid.h"

mcsup_nodeconfig_t mc_config;
int mc_config_initialized = 0;

int main(int argc, char ** argv){
  MPI_Init(&argc, &argv);
	sleep(1);
  MPI_Finalize();
  return 0;
}
