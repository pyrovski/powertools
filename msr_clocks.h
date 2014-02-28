#ifndef MSR_CLOCKS_H
#define MSR_CLOCKS_H

void read_aperf_mperf(int core, uint64_t *aperf, uint64_t *mperf); 

#endif //MSR_CLOCKS_H
