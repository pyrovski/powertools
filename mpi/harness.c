#include <unistd.h>
#include <mpi.h>

mcsup_nodeconfig_t mc_config;
int mc_config_initialized = 0;

int
main(int argc, char **argv){
	int rank, size;
	MPI_Init(&argc, &argv);
	sleep(10);
	MPI_Finalize();
	return 0;
}

