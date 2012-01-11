all:	end2end end2endall query clamp_70W clamp_65W clamp_60W clamp_55W clamp_53W clamp_52W

clamp_70W:
	./wrap.py -f -g -o clamp.c clamp.w
	mpicc -DCLAMP_70W -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c clamp.c -o clamp_70W.o
	mpicc -shared -Wl,-soname,libclamp_70W.so -o ../lib/libclamp_70W.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o clamp_70W.o 
clamp_65W:
	./wrap.py -f -g -o clamp.c clamp.w
	mpicc -DCLAMP_65W -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c clamp.c -o clamp_65W.o
	mpicc -shared -Wl,-soname,libclamp_65W.so -o ../lib/libclamp_65W.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o clamp_65W.o 
clamp_60W:
	./wrap.py -f -g -o clamp.c clamp.w
	mpicc -DCLAMP_60W -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c clamp.c -o clamp_60W.o
	mpicc -shared -Wl,-soname,libclamp_60W.so -o ../lib/libclamp_60W.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o clamp_60W.o 
clamp_55W:
	./wrap.py -f -g -o clamp.c clamp.w
	mpicc -DCLAMP_55W -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c clamp.c -o clamp_55W.o
	mpicc -shared -Wl,-soname,libclamp_55W.so -o ../lib/libclamp_55W.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o clamp_55W.o 
clamp_53W:
	./wrap.py -f -g -o clamp.c clamp.w
	mpicc -DCLAMP_53W -DPIC -fPIC -I.. -DARCH_SANDY_BRIDGE -DARCH_062D -c clamp.c -o clamp_53W.o
	mpicc -shared -Wl,-soname,libclamp_53W.so -o ../lib/libclamp_53W.so ../msr_core.o ../msr_rapl.o ../msr_turbo.o ../blr_util.o clamp_53W.o 
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


