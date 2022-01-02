#!/bin/bash

# This script simply executes a batch job to run the problem over and over.
# As the runtime is very long, consider executing it in a GNU screen session.
# All parameters to this script are passed to the vrptwms call.
# The output will be an analysis of the maximum memory usage.

unalias grep 2> /dev/null  # make sure there's no alias for grep

runs=5  # number of runs
seconds=900
out=memory

for ((run=0; run<runs; run++)); do
  parallel --gnu /usr/bin/time -v ./vrptwms -r $seconds --parallel -m aco $@ {} 2>&1 ::: data/R1??.txt | grep Max >> $out.aco.txt
  parallel --gnu /usr/bin/time -v ./vrptwms -r $seconds --parallel -m grasp $@ {} 2>&1 ::: data/R1??.txt | grep Max >> $out.grasp.txt
  echo >> $out.aco.txt
  echo >> $out.grasp.txt
done
