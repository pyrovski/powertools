/****************************************************************************************************/
/****************************************************************************************************/
/* clamp.c automatically created by clamp.w							    */
/****************************************************************************************************/
/****************************************************************************************************/
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include "msr_core.h"
#include "msr_rapl.h"
#include "msr_opt.h"
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
static FILE* f;

static uint64_t original_power_limit[NUM_PACKAGES][NUM_DOMAINS];
static uint64_t new_power_limit[NUM_PACKAGES][NUM_DOMAINS];

{{fn foo MPI_Init}}
	gethostname( hostname, 1024 );
	{{callfn}}
	rank = -1;
	PMPI_Comm_rank( MPI_COMM_WORLD, &rank );
	PMPI_Barrier( MPI_COMM_WORLD );
	
	if(rank == 0){
		f = safe_mkstemp(hostname, "end2end", rank);
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

		get_raw_power_limit( 0, PKG_DOMAIN, &original_pkg_power_limit[0] );
		get_raw_power_limit( 1, PKG_DOMAIN, &original_pkg_power_limit[1] );

		parse_opts( *argc, *argv );
		
		
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
		fprintf(f, "QQQ %5s %3.8lf %5.10lf %5.10lf %5.10lf %5.10lf 0x%0lx 0x%0lx %s\n",
			hostname,
			elapsed,
			joules[0][PKG_DOMAIN],
			joules[1][PKG_DOMAIN],
			joules[0][PP0_DOMAIN],
			joules[1][PP0_DOMAIN],
			original_pkg_power_limit[0],
			original_pkg_power_limit[1],
			getenv("SLURM_JOB_NAME"));
	}
	PMPI_Barrier( MPI_COMM_WORLD );
	{{callfn}}
{{endfn}}
