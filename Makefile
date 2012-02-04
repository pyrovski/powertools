# Note:  Sandy Bridge Core is -DARCH_062A
# 	 Sandy Bridge Xeon is -DARCH_062D

target=msr
library=libmsr.so

DEFINES=-DTEST_HARNESS -DARCH_SANDY_BRIDGE -DARCH_062D -DPKG_PERF_STATUS_AVAILABLE

ifneq ($(dbg),)
DEFINES +=-D_DEBUG=$(dbg) -g -pg
endif

CFLAGS=-fPIC -Wall ${DEFINES}
CC=gcc

all: $(target) $(library)

msr_common.o: msr_rapl.h msr_common.c msr_common.h Makefile
msr_core.o: msr_core.c msr_core.h Makefile
msr_rapl.o: msr_core.h msr_rapl.c msr_rapl.h Makefile
msr_pebs.o: msr_core.h msr_pebs.c msr_pebs.h Makefile
msr_turbo.o: msr_core.h msr_turbo.c msr_turbo.h Makefile
msr_opt.o: msr_core.h msr_rapl.h msr_opt.c msr_opt.h Makefile
blr_util.o: blr_util.h blr_util.c Makefile

$(target): msr_rapl.o msr_core.o msr_common.o msr_pebs.o blr_util.o msr_turbo.o msr_opt.o blr_util.o
	gcc -fPIC -Wall ${DEFINES} -o $(target) msr_pebs.c msr_rapl.o msr_common.o msr_core.o msr_opt.o blr_util.o

install: $(library)
	install -m 0644 -t $(HOME)/local/include msr_rapl.h msr_core.h blr_util.h
	install -m 0644 -t $(HOME)/local/lib $(library)

$(library): msr_rapl.o blr_util.o msr_core.o msr_turbo.o msr_pebs.o
	gcc -shared -Wl,-soname,$(library) -o $(library) $^

clean:
	rm -f *.o $(target) $(library)

zin:
	srun -n 1 -p asde ./msr
