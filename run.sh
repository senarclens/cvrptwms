#!/bin/bash

# This script simply executes a batch job to run the problem over and over.
# As the runtime is very long, consider executing it in a GNU screen session.
# All parameters to this script are passed to the vrptwms call.

runs=5  # number of runs
metaheuristic=aco

# Execute the command for runtimes from 15 seconds to 16 minutes.
for i in {0..6}; do
  seconds=$((15 * 2 ** $i))
  echo $seconds
  for ((run=0; run<runs; run++)); do
    parallel --gnu cvrptwms -r $seconds --parallel -m $metaheuristic $@ >> out.$seconds ::: data/R1??.txt
    echo >> out.$seconds
#     echo $run
  done
done
