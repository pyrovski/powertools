all:	end2end end2endall query clamp_52W rapl

rapl:
	./wrap.py -f -g -o rapl.c rapl.w
	mpicc -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c rapl.c -o rapl.o
	mpicc -shared -Wl,-soname,librapl.so -o ../lib/librapl.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o ../msr_opt.o rapl.o 
	mpicc -L../lib -o harness.rapl harness.c -lrapl

clamp_52W:
	./wrap.py -f -g -o clamp.c clamp.w
	mpicc -DCLAMP_52W -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c clamp.c -o clamp_52W.o
	mpicc -shared -Wl,-soname,libclamp_52W.so -o ../lib/libclamp_52W.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o clamp_52W.o 
end2end:
	./wrap.py -f -g -o end2end.c end2end.w
	mpicc -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c end2end.c 
	mpicc -shared -Wl,-soname,libend2end.so -o ../lib/libend2end.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o end2end.o 
	mpicc -L../lib -o harness.end2end harness.c -lend2end
end2endall:
	./wrap.py -f -g -o end2endall.c end2endall.w
	mpicc -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c end2endall.c 
	mpicc -shared -Wl,-soname,libend2endall.so -o ../lib/libend2endall.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o end2endall.o 
	mpicc -L../lib -o harness.end2endall harness.c -lend2endall
query:
	./wrap.py -f -g -o query.c query.w
	mpicc -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c query.c 
	mpicc -shared -Wl,-soname,libquery.so -o ../lib/libquery.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o query.o 
	mpicc -L../lib -o harness.query harness.c -lquery
zin:
	srun -n 1 --tasks-per-node=1 -p asde ./harness.end2end 
zend2endall:
	srun -n 32 --tasks-per-node=1 -p asde ./harness.end2endall
zquery:
	srun -N 1 -n 1 -p asde ./harness.query > query.out 2>&1

zrapl:
	srun -N 1 -n 1 -p asde ./harness.rapl > rapl.out 2>&1

