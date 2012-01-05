end2end:
	./wrap.py -g -o end2end.c end2end.w
	mpicc -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c end2end.c
	mpicc -shared -Wl,-soname,libend2end.so -o ../lib/libend2end.so ../msr_core.o ../msr_rapl.o ../blr_util.o end2end.o
	mpicc -o harness harness.c 
	mpicc -L../lib -o harness.end2end harness.c -lend2end
zin:
	srun -n 32 --tasks-per-node=1 -p asde ./harness.end2end 

