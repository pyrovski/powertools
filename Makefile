target=msr
library=libmsr.so

DEFINES=-DTEST_HARNESS -DARCH_SANDY_BRIDGE -DPKG_PERF_STATUS_AVAILABLE

# Machine- and compiler-specific information goes here:
-include localConfig.d/localConfig

ifneq ($(dbg),)
DEFINES +=-D_DEBUG=$(dbg) -g -pg
else
DEFINES +=-O2
endif

CFLAGS=-fPIC -Wall ${DEFINES} ${COMPILER_SPECIFIC_FLAGS}
CC=mpicc

.PHONY: mpi

all: $(target) $(library) turbo rapl_clamp mpi

mpi:
	$(MAKE) -C mpi

msr_common.o: Makefile            msr_rapl.o msr_common.c msr_common.h 
msr_core.o:   Makefile                       msr_core.c   msr_core.h 
msr_rapl.o:   Makefile msr_core.o            msr_rapl.c   msr_rapl.h 
msr_pebs.o:   Makefile msr_core.o            msr_pebs.c   msr_pebs.h 
msr_turbo.o:  Makefile msr_core.o            msr_turbo.c  msr_turbo.h 
msr_opt.o:    Makefile msr_core.o msr_rapl.o msr_opt.c    msr_opt.h 
msr_clocks.o: Makefile msr_core.o            msr_clocks.h msr_clocks.h
blr_util.o:   Makefile                       blr_util.h   blr_util.c 
cpuid.o:      Makefile cpuid.h
rapl_poll.o:  Makefile rapl_poll.c
sample.o:     Makefile sample.c

$(target): msr_rapl.o msr_core.o msr_common.o msr_pebs.o blr_util.o msr_turbo.o\
 msr_opt.o blr_util.o cpuid.o rapl_poll.o sample.o
	$(CC) -fPIC -Wall ${DEFINES} -o $(target) $^ -lrt

install: $(library) $(target) msr_rapl.h msr_core.h blr_util.h msr_common.h turbo rapl_clamp plot.R parse_rapl.sh
	install -m 0644 -t $(HOME)/local/include/ msr_rapl.h msr_core.h\
 blr_util.h msr_common.h
	install -m 0644 -t $(HOME)/local/lib/ $(library)
	install -m 0744 -t $(HOME)/local/bin/ turbo rapl_clamp $(target) plot.R parse_rapl.sh

$(library): msr_rapl.o blr_util.o msr_core.o msr_turbo.o msr_pebs.o msr_opt.o msr_clocks.o
	$(CC) -shared -Wl,-soname,$(library) -o $(library) $^

turbo: turbo.c cpuid.c msr_turbo.c msr_core.c
	$(CC) $(CFLAGS) -o $@ $^

rapl_clamp: rapl_clamp.c msr_core.c cpuid.c msr_rapl.c msr_opt.c blr_util.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o $(target) $(library) turbo rapl_clamp
