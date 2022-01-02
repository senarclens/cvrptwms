#!/bin/bash
# tiny script to test if "double free or corruption" occurs in 100 executions

for i in {1..500}
do
    OUT=`./vrptwms ../vrptwms/data/R101.txt --seed=$i --ls 0 -m 0`
done
echo PASS if no output, otherwise fail
