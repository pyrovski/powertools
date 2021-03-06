#ifndef MSR_CORE_H
#define MSR_CORE_H
#include <stdint.h>
#include <sys/types.h>	// off_t
#include "cpuid.h"
#define MAX_NUM_PACKAGES 4
enum{
	MSR_AND,
	MSR_OR,
	MSR_XOR
};

extern mcsup_nodeconfig_t mc_config;
extern int mc_config_initialized;

void init_msr();
void finalize_msr();
void write_msr(int core, off_t msr, uint64_t val);
void read_msr(int core, off_t msr, uint64_t *val);
void read_modify_write_msr( int core, off_t msr, uint64_t mask, int op );
void write_and_validate_msr( int core, off_t msr, uint64_t val );
#endif //MSR_CORE_H
