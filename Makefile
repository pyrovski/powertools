# Note:  Sandy Bridge Core is -DARCH_062A
# 	 Sandy Bridge Xeon is -DARCH_062D

target=msr

DEFINES=-DTEST_HARNESS -DARCH_SANDY_BRIDGE -DARCH_062D -DPKG_PERF_STATUS_AVAILABLE

<<<<<<< HEAD
ifneq ($(dbg),)
DEFINES +=-D_DEBUG=$(dbg) -g -pg
endif

$(target): msr_rapl msr_core msr_common msr_pebs blr_util msr_turbo
	mpicc -fPIC -Wall ${DEFINES} -o $(target) msr_pebs.c msr_rapl.o msr_common.o msr_core.o 
=======
test_harness: msr_rapl msr_core msr_common msr_pebs blr_util msr_turbo msr_opt blr_util
	mpicc -fPIC -Wall ${DEFINES} -o msr msr_opt.o msr_pebs.c msr_rapl.o msr_common.o msr_core.o blr_util.o
>>>>>>> ea24572b3912346e8ea05e5a677f5956f8c4b915
msr_common: msr_rapl msr_common.c msr_common.h Makefile
	mpicc -fPIC -Wall ${DEFINES} -c msr_common.c
msr_core: msr_core.c msr_core.h Makefile
	mpicc -fPIC -Wall ${DEFINES} -c msr_core.c
msr_rapl: msr_core msr_rapl.c msr_rapl.h Makefile
	mpicc -fPIC -Wall ${DEFINES} -c msr_rapl.c
msr_pebs: msr_core msr_pebs.c msr_pebs.h Makefile
	mpicc -fPIC -Wall ${DEFINES} -c msr_pebs.c
msr_turbo:  msr_core msr_turbo.c msr_turbo.h Makefile
	mpicc -fPIC -Wall ${DEFINES} -c msr_turbo.c
msr_opt: msr_core msr_rapl msr_opt.c msr_opt.h Makefile
	mpicc -fPIC -Wall ${DEFINES} -c msr_opt.c
blr_util: blr_util.h blr_util.c Makefile
	mpicc -fPIC -Wall ${DEFINES} -c blr_util.c
clean:
	rm -f *.o $(target)

lib:
	mkdir -p lib

zin:
	srun -n 1 -p asde ./msr

