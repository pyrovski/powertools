#ifndef SAMPLE_H
#define SAMPLE_H
void installSampleHandler(int *flag);
void msSample(const char * const filename, int log, double interval,
							int numSockets,
							struct rapl_state_s *rapl_state);
#endif
