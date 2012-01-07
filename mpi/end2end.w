/****************************************************************************************************/
/****************************************************************************************************/
/* end2end.c automatically created by end2end.w							    */
/****************************************************************************************************/
/****************************************************************************************************/
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include "msr_core.h"
#include "msr_rapl.h"
#include "blr_util.h"

static struct power_units units[2];
static struct power_info info[NUM_DOMAINS];
static double joules[2][NUM_DOMAINS];
static struct power_limit limit[NUM_DOMAINS];
static struct timeval start, finish;
static double status_sec;
static uint64_t policy;
static int rank;
static char hostname[1025];
extern int msr_debug;

{{fn foo MPI_Init}}
	gethostname( hostname, 1024 );
	{{callfn}}
	rank = -1;
	PMPI_Comm_rank( MPI_COMM_WORLD, &rank );
	PMPI_Barrier( MPI_COMM_WORLD );
	if(rank == 0){
		init_msr();
		disable_turbo(0);
		disable_turbo(1);
		sleep(1);
		get_rapl_power_unit(0, &units[0]);
		get_rapl_power_unit(1, &units[1]);
		get_energy_status( 0, PKG_DOMAIN, &joules[0][PKG_DOMAIN], &units[0] );
		get_energy_status( 1, PKG_DOMAIN, &joules[1][PKG_DOMAIN], &units[1] );
		get_energy_status( 0, PP0_DOMAIN, &joules[0][PP0_DOMAIN], &units[0] );
		get_energy_status( 1, PP0_DOMAIN, &joules[1][PP0_DOMAIN], &units[1] );
		gettimeofday( &start, NULL );
	}
{{endfn}}

{{fn foo MPI_Finalize}}
	double elapsed;
	if(rank == 0){
		gettimeofday( &finish, NULL );
		get_energy_status( 0, PKG_DOMAIN, &joules[0][PKG_DOMAIN], &units[0] );
		get_energy_status( 1, PKG_DOMAIN, &joules[1][PKG_DOMAIN], &units[1] );
		get_energy_status( 0, PP0_DOMAIN, &joules[0][PP0_DOMAIN], &units[0] );
		get_energy_status( 1, PP0_DOMAIN, &joules[1][PP0_DOMAIN], &units[1] );
		enable_turbo(0);
		enable_turbo(1);
		elapsed = ts_delta( &start, &finish );
		fprintf(stderr, "QQQ %5s %3.8lf %5.10lf %5.10lf %5.10lf %5.10lf\n",
			hostname,
			elapsed,
			joules[0][PKG_DOMAIN],
			joules[1][PKG_DOMAIN],
			joules[0][PP0_DOMAIN],
			joules[1][PP0_DOMAIN]);
	}
	PMPI_Barrier( MPI_COMM_WORLD );
	{{callfn}}
{{endfn}}
