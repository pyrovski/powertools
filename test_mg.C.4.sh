#!/bin/bash

# 1st arg = pkg power limit in watts
# 2nd arg = raw pkg timing window
# no args = no limit
nprocs=4
name=mg.C.$nprocs

#disable OS DVFS
#sudo bash -c 'echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor'

oldDir=`pwd`

# enable rapl with provided settings
rapl_clamp -d
rapl_clamp -e -P $1 -w $2

cd $HOME/code/GreenMPI/NPB3.3/NPB3.3-MPI/MG
# start logging
msr $oldDir/${name}_P`printf "%03u" $1`_w`printf "%02u" $2`.dat &

sleep .01s

# run benchmark
#likwid-perfctr -g ENERGY -c S0:0-3 -m likwid-bench -i 500000 -g 1 -t peakflops_avx -w S0:5MB > bench_P`printf "%03u" $1`_w`printf "%02u" $2`.log 2>&1
mpirun -n $nprocs ../bin/mg.C.$nprocs >$oldDir/${name}_P`printf "%03u" $1`_w`printf "%02u" $2`.log

# stop logging
for job in `jobs -p`
do
#    echo $job
    kill $job
    wait $job
done

# plot logged data with provided settings in filename
plot.R $oldDir/${name}_P`printf "%03u" $1`_w`printf "%02u" $2`.dat $oldDir/${name}_P`printf "%03u" $1`_w`printf "%02u" $2`.pdf $name
