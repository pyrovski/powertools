all:	rapl


../lib:
	mkdir -p ../lib

rapl: ../lib rapl.w wrap.py
	mkdir -p ../lib
	./wrap.py -f -g -o rapl.c rapl.w
	mpicc -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c rapl.c -o rapl.o
	mpicc -shared -Wl,-soname,librapl.so -o ../lib/librapl.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o ../msr_opt.o rapl.o 

