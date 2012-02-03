all:	rapl

rapl:
	./wrap.py -f -g -o rapl.c rapl.w
	mpicc -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c rapl.c -o rapl.o
	mpicc -shared -Wl,-soname,librapl.so -o ../lib/librapl.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o ../msr_opt.o rapl.o 
	mpicc -L../lib -o harness.rapl harness.c -lrapl

noturbo:
	./wrap.py -f -g -o noturbo.c noturbo.w
	mpicc -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c noturbo.c -o noturbo.o
	mpicc -shared -Wl,-soname,libnoturbo.so -o ../lib/libnoturbo.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o ../msr_opt.o noturbo.o 
	mpicc -L../lib -o harness.noturbo harness.c -lnoturbo


zrapl:
	srun -N 1 -n 1 -p asde ./harness.rapl > rapl.out 2>&1

