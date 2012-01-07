#ifndef MSR_CORE_H
#define MSR_CORE_H
#include <stdint.h>
#include <sys/types.h>	// off_t
#define NUM_PACKAGES 2
#define NUM_CORES_PER_PACKAGE 8
enum{
	MSR_AND,
	MSR_OR,
	MSR_XOR
};
void init_msr();
void finalize_msr();
void write_msr(int cpu, off_t msr, uint64_t val);
void read_msr(int cpu, off_t msr, uint64_t *val);
void read_modify_write_msr( int cpu, off_t msr, uint64_t mask, int op );
void write_and_validate_msr( int cpu, off_t msr, uint64_t val );
#endif //MSR_CORE_H
