#!/usr/bin/env python3

"""
Hack to generate csv file w/ one result per second from given output.
"""

import argparse
import csv
import os.path
import sys

MAX_SECONDS = 960
OUT_SUFFIX = '_seconds.csv'

def parse_line(line):
    """
    Return the result value and time to obtain it.
    """
    val = float(line.split()[4])
    sec = int(line.split()[5][1:])
    return val, sec


def update_run(run, last_second, sec, last_val):
    """
    Write last_val from last_second to (not including) sec to the run dict.
    """
    while last_second < sec:
        last_second += 1
        run[last_second] = last_val


def write_runs(runs, fname):
    """
    Write the given runs as csv to fname.
    """
    with open(fname, 'w', encoding="utf=8") as f:
        csv_writer = csv.writer(f)
        csv_writer.writerow(range(1, len(runs) + 1))
        for key in runs[0]:
            csv_writer.writerow([run[key] for run in runs])


def process_file(filename):
    """
    Read the given file and write corresponding output.
    """
    outfname = os.path.splitext(filename)[0] + OUT_SUFFIX
    runs = []
    lastline = None

    with open(filename, encoding="utf-8") as f:
        for l in f:
            if '->' not in l:  # skip lines w/out data
                if lastline is 'data':
                    update_run(run, last_second, MAX_SECONDS, last_val)
                    runs.append(run)
                lastline = 'text'
                continue
            if lastline is not 'data':  # initialize new run
                run = {}
                last_val = None
                last_second = 0
                lastline = 'data'
            val, sec = parse_line(l)
            if sec == last_second:
                last_val = val
                continue
            update_run(run, last_second, sec, last_val)
            last_second = sec
            last_val = val
    write_runs(runs, outfname)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('infile', nargs='+',
                        help='one or more input files')
    args = parser.parse_args()
    for infile in args.infile:
        process_file(infile)


if __name__ == "__main__":
    sys.exit(main())
