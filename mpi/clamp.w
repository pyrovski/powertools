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

uint64_t original_pkg_power_limit[2];

#ifdef CLAMP_52W
uint64_t test_pkg_power_limit = 0x781a0001581a0LL;	
#endif
#ifdef CLAMP_53W
uint64_t test_pkg_power_limit = 0x781a8001581a8LL;	
#endif
#ifdef CLAMP_55W
uint64_t test_pkg_power_limit = 0x781b8001581b8LL;	
#endif
#ifdef CLAMP_60W
uint64_t test_pkg_power_limit = 0x781e0001581e0LL;	
#endif
#ifdef CLAMP_65W
uint64_t test_pkg_power_limit = 0x7820800158208LL;	
#endif
#ifdef CLAMP_70W
uint64_t test_pkg_power_limit = 0x7823000158230LL;	
#endif

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

		set_raw_power_limit( 0, PKG_DOMAIN, test_pkg_power_limit );
		set_raw_power_limit( 1, PKG_DOMAIN, test_pkg_power_limit );

		
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
