#!/bin/bash

name=peakflops_avx

# enable rapl with provided settings
rapl_clamp -d
rapl_clamp -e -P $1 -w $2

# start logging
msr output_P`printf "%03u" $1`_w`printf "%02u" $2`.dat &

sleep .01s

# run benchmark
likwid-perfctr -g ENERGY -c S0:0-3 -m likwid-bench -i 500000 -g 1 -t $name -w S0:5MB > bench_P`printf "%03u" $1`_w`printf "%02u" $2`.log 2>&1

# stop logging
for job in `jobs -p`
do
#    echo $job
    kill $job
    wait $job
done

# plot logged data with provided settings in filename
./plot.R output_P`printf "%03u" $1`_w`printf "%02u" $2`.dat plot_P`printf "%03u" $1`_w`printf "%02u" $2`.pdf $name
