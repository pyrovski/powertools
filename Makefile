target=msr
library=libmsr.so

DEFINES=-DTEST_HARNESS -DARCH_SANDY_BRIDGE -DPKG_PERF_STATUS_AVAILABLE
CC=gcc

# Machine- and compiler-specific information goes here:
-include localConfig.d/localConfig

INSTALL_DEST?=$(HOME)/local

ifneq ($(dbg),)
DEFINES +=-D_DEBUG=$(dbg) -g -pg
else
DEFINES +=-O2 -DNDEBUG
endif

CFLAGS=-fPIC -Wall ${DEFINES} ${COMPILER_SPECIFIC_FLAGS}

.PHONY: mpi

all: $(target) $(library) turbo rapl_clamp mpi

mpi:
	$(MAKE) -C mpi

msr_common.o: Makefile            msr_rapl.o msr_common.c msr_common.h 
msr_core.o:   Makefile                       msr_core.c   msr_core.h 
msr_rapl.o:   Makefile msr_core.o            msr_rapl.c   msr_rapl.h 
msr_pebs.o:   Makefile msr_core.o            msr_pebs.c   msr_pebs.h 
msr_turbo.o:  Makefile msr_core.o            msr_turbo.c  msr_turbo.h 
msr_clocks.o: Makefile msr_core.o            msr_clocks.h msr_clocks.h
blr_util.o:   Makefile                       blr_util.h   blr_util.c 
cpuid.o:      Makefile cpuid.h
rapl_poll.o:  Makefile rapl_poll.c
sample.o:     Makefile sample.c

$(target): msr_rapl.o msr_core.o msr_common.o msr_pebs.o blr_util.o msr_turbo.o\
 blr_util.o cpuid.o rapl_poll.o sample.o
	$(CC) -fPIC -Wall ${DEFINES} -o $(target) $^ -lrt -lm

install: $(library) $(target) msr_rapl.h msr_core.h blr_util.h msr_common.h turbo rapl_clamp plot.R parse_rapl.sh msr_clocks.h
	mkdir -p $(INSTALL_DEST)/include $(INSTALL_DEST)/lib $(INSTALL_DEST)/bin
	install -m 0644 -t $(INSTALL_DEST)/include/ msr_rapl.h msr_core.h\
 blr_util.h msr_common.h msr_clocks.h
	install -m 0644 -t $(INSTALL_DEST)/lib/ $(library)
	install -m 0744 -t $(INSTALL_DEST)/bin/ turbo rapl_clamp $(target) plot.R parse_rapl.sh

$(library): msr_rapl.o blr_util.o msr_core.o msr_turbo.o msr_pebs.o msr_clocks.o
	$(CC) -shared -Wl,-soname,$(library) -o $(library) $^

turbo: turbo.o cpuid.o msr_turbo.o msr_core.o
	$(CC) $(CFLAGS) -o $@ $^

rapl_clamp: rapl_clamp.o msr_core.o cpuid.o msr_rapl.o blr_util.o sample.o
	$(CC) $(CFLAGS) -o $@ $^ -lrt -lm

clean:
	rm -f *.o $(target) $(library) turbo rapl_clamp
