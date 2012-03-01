#!/bin/bash

# enable rapl with provided settings
rapl_clamp -d
rapl_clamp -e -P $1 -w $2

# start logging
./msr output_P$1_w$2.dat &

# run benchmark
#likwid-perfctr -g ENERGY -c S0:0-3 -m likwid-bench -i 500000 -g 1 -t peakflops_avx -w S0:5MB

sleep 2

# stop logging
for job in `jobs -p`
do
    echo $job
    kill $job
    wait $job || exit(-1)
done

# plot logged data with provided settings in filename
./plot.R output_P$1_w$2.dat plot_P$1_w$2.pdf
