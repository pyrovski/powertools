#ifndef MSR_FREQ_H
#define MSR_FREQ_H
/*
  MSR_IA32_MPERF
  MSR_IA32_APERF
*/

void read_aperf_mperf(int cpu, uint64_t *aperf, uint64_t *mperf);

#endif
