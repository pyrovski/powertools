test_harness: msr_rapl msr_core msr_common 
	gcc -Wall -o msr msr_rapl.o msr_common.o msr_core.o 
msr_common: msr_rapl msr_common.c msr_common.h Makefile
	gcc -Wall -DTEST_HARNESS -c msr_common.c
msr_core: msr_core.c msr_core.h Makefile
	gcc -Wall -c msr_core.c
msr_rapl: msr_core msr_rapl.c msr_rapl.h Makefile
	gcc -Wall -DARCH_SANDY_BRIDGE -DARCH_062A -c msr_rapl.c
clean:
	rm -f *.o


