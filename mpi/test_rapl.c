#include <mpi.h>

int main(int argc, char ** argv){
  MPI_Init(&argc, &argv);
	sleep(1);
  MPI_Finalize();
  return 0;
}
