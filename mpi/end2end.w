/****************************************************************************************************/
/****************************************************************************************************/
/* end2end.c automatically created by end2end.w							    */
/****************************************************************************************************/
/****************************************************************************************************/
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "msr_core.h"
#include "msr_rapl.h"
#include "blr_util.h"

static struct power_units units;
static struct power_info info[NUM_DOMAINS];
static double joules[NUM_DOMAINS];
static struct timeval start, finish;
static int rank;
static char hostname[1025];

{{fn foo MPI_Init}}
	gethostname( hostname, 1024 );
	{{callfn}}
	gettimeofday(&start, NULL);
	init_msr();
	get_rapl_power_unit(0, &units);
	get_power_info(0, PKG_DOMAIN, &info[PKG_DOMAIN],&units);
        get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units);
        get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units);
        get_energy_status(0, DRAM_DOMAIN, &joules[DRAM_DOMAIN], &units);
	PMPI_Comm_rank( MPI_COMM_WORLD, &rank );
{{endfn}}
{{fn foo MPI_Finalize}}
        get_energy_status(0, PKG_DOMAIN, &joules[PKG_DOMAIN], &units);
        get_energy_status(0, PP0_DOMAIN, &joules[PP0_DOMAIN], &units);
        get_energy_status(0, DRAM_DOMAIN, &joules[DRAM_DOMAIN], &units);
	gettimeofday(&finish, NULL);
	sleep(rank);	// Extra cheesey.
	fprintf(stderr, "hostname= %10s  rank= %3d  elapsed_sec= %lf  pkg_J= %15.10lf  pp0_J= %15.10lf  dram_J= %15.10lf\n",
		hostname,
		rank,
		ts_delta(&start, &finish), 
		joules[PKG_DOMAIN],joules[PP0_DOMAIN],joules[DRAM_DOMAIN] );
	{{callfn}}
{{endfn}}
