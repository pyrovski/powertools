/* msr_pebs.c
 */
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <sys/types.h>
#include "msr_pebs.h"
#include "msr_core.h"

static const off_t IA32_DS_AREA		= 0x600;
static const off_t IA32_PEBS_ENABLE 	= 0x3f1;

void
pebs_init(int nRecords, uint64_t *counter, uint64_t *reset_val ){
	// 1. Set up the precise event buffering utilities.
	// 	a.  Place values in the
	// 		i.   precise event buffer base,
	//		ii.  precise event index
	//		iii. precise event absolute maximum,
	//		iv.  precise event interrupt threshold,
	//		v.   and precise event counter reset fields
	//	    of the DS buffer management area.
	//
	// 2.  Enable PEBS.  Set the Enable PEBS on PMC0 flag 
	// 	(bit 0) in IA32_PEBS_ENABLE_MSR.
	//
	// 3.  Set up the IA32_PMC0 performance counter and 
	// 	IA32_PERFEVTSEL0 for an event listed in Table 
	// 	18-10.
	
	// IA32_DS_AREA points to 0x58 bytes of memory.  
	// (11 entries * 8 bytes each = 88 bytes.)
	
	// Each PEBS record is 0xB0 byes long.
	
	
	struct ds_area *pds_area = mmap(
			NULL,			// let kernel choose address
			sizeof(struct ds_area),	// keep ds and records separate.
			PROT_READ | PROT_WRITE, 
			MAP_ANONYMOUS | MAP_LOCKED,
			-1,			// dummy file descriptor
			0);			// offset (ignored).
	assert(ds_area != (void*)-1);

	struct pebs *ppebs = mmap(
			NULL,
			sizeof(struct pebs)*nRecords, 
			PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_LOCKED,
			-1,
			0);
	assert(ppebs != (void*)-1);

	pds_area->bts_buffer_base 		= 0;
	pds_area->bts_index			= 0;
	pds_area->bts_abolute_maximum		= 0;
	pds_area->bts_interrupt_threshold	= 0;
	pds_area->pebs_buffer_base		= ppebs;
	pds_area->pebs_index			= ppebs;
	pds_area->pebs_absolute_maximum		= ppebs + (nRecords-1) * sizeof(struct pebs);
	pds_area->pebs_interrupt_threshold	= ppebs + (nRecords+1) * sizeof(struct pebs);
	pds_area->pebs_counter0_reset		= reset_val[0];
	pds_area->pebs_counter1_reset		= reset_val[1];
	pds_area->pebs_counter2_reset		= reset_val[2];
	pds_area->pebs_counter3_reset		= reset_val[3];
	pds_area->reserved			= 0;

	write_msr(0, PERF_GLOBAL_CTRL, 0);			// known good state.
	write_msr(0, IA32_DS_AREA, (uint64_t)ds_area);
	write_msr(0, IA32_PEBS_ENABLE, 0xf | (0xf << 32) );	// Figure 18-14.

	write_msr(0, PMC0, reset_val[0]);
	write_msr(1, PMC1, reset_val[1]);
	write_msr(2, PMC2, reset_val[2]);
	write_msr(3, PMC3, reset_val[3]);

	write_msr(0, IA32_PERFEVTSEL0, 0x410000 | counter[0]);
	write_msr(0, IA32_PERFEVTSEL1, 0x410000 | counter[1]);
	write_msr(0, IA32_PERFEVTSEL2, 0x410000 | counter[2]);
	write_msr(0, IA32_PERFEVTSEL3, 0x410000 | counter[3]);

	write_msr(0, PERF_GLOBAL_CTRL, 0xf);

	stomp();

	write_msr(0, PERF_GLOBAL_CTRL, 0x0);



	// note AnyThread, Edge, Invert and CMask must all be 0.
	


