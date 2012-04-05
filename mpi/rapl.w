#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include "msr_core.h"
#include "msr_rapl.h"
#include "msr_opt.h"
#include "blr_util.h"

static int rank;
static char hostname[1025];
extern int msr_debug;
static FILE* f;
struct rapl_state_s s;

static int msr_rank_mod=1;

{{fn foo MPI_Init}}
	{{callfn}}
	rank = -1;
	PMPI_Comm_rank( MPI_COMM_WORLD, &rank );
	get_env_int("MSR_RANK_MOD", &msr_rank_mod);
	if(rank%msr_rank_mod == 0){
		gethostname( hostname, 1024 );
		f = safe_mkstemp(hostname, "rapl", rank);
		init_msr();
		disable_turbo(0);
		disable_turbo(1);
		rapl_init(&s, f ,1);
	}
{{endfn}}

{{fn foo MPI_Finalize}}
	double elapsed;
	if(rank%msr_rank_mod == 0){
		rapl_finalize(&s, 1);
	}
	{{callfn}}
{{endfn}}
