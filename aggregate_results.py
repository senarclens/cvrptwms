#!/usr/bin/env python3

"""
Script that automatizes analyzing vrptwms results in csv format.
The script takes an out.csv file and outputs the best, average and worst result
for each problem instance.
"""

import argparse
import csv
import os
import sys

VERBOSE = False
LATEX = False
NAME = "name"
TRUCKS = "trucks"
WORKERS = "workers"
DISTANCE = "distance"
COST = "cost"
DURATION = "duration"
BKS_FILE = "vrptwms_bks.csv"  # best known solutions
FIELDS = (NAME, TRUCKS, WORKERS, DISTANCE, COST, DURATION)


class Aggregated_Result(object):
    """
    Represents the best, average and worst of each instance.
    """
    def __init__(self, result):
        self.name = result.pop(NAME)
        self.count = 1
        self.best = Result(result)
        self.worst = Result(result)
        self.all = [Result(result), ]

    def __str__(self):
        if VERBOSE:
            if self.count == 1:
                s = "%s (1 run)%s" % (self.name, os.linesep)
            else:
                s = "%s (%d runs)%s" % (self.name, self.count, os.linesep)
            s += "best:    %s%s" % (self.best, os.linesep)
            s += "average: %s%s" % (self.average(), os.linesep)
            s += "worst:   %s%s" % (self.worst, os.linesep)
        else:
            s = "%s, %s, %d run[s]" % (self.name, self.best, self.count)
        return s

    def add(self, result):
        """
        Add a single result to the analysis of this instance.
        """
        result.pop(NAME)
        self.count += 1
        if result[COST] < self.best[COST]:
            self.best = Result(result)
        if result[COST] > self.worst[COST]:
            self.worst = Result(result)
        self.all.append(result)

    def _average(self, field):
        return sum([x[field] for x in self.all]) / float(self.count)

    def average(self):
        return Result({TRUCKS: self._average(TRUCKS),
                       WORKERS: self._average(WORKERS),
                       DISTANCE: self._average(DISTANCE),
                       COST: self._average(COST),
                       DURATION: self._average(DURATION)})


class Result(dict):
    """
    Represents a single instance's result.
    """
    def __init__(self, result=None):
        if not result:
            result = {TRUCKS: 0, WORKERS: 0, DISTANCE: 0, COST: 0,
                      DURATION: 0}
        result[TRUCKS] = float(result[TRUCKS])
        result[WORKERS] = float(result[WORKERS])
        result[DISTANCE] = float(result[DISTANCE])
        result[COST] = float(result[COST])
        result[DURATION] = float(result[DURATION])
        self.store = dict(result)
        self.update(dict(result))

    def __str__(self):
        if LATEX:
            return "%4.1f & %.1f & %7.2f & %.2f & %d\\\\" % (
                self.store[TRUCKS],
                self.store[WORKERS],
                self.store[DISTANCE],
                self.store[COST],
                self.store[DURATION])
        return "%.1f, %.1f, %.2f, %.6f, %d" % (self.store[TRUCKS],
                                               self.store[WORKERS],
                                               self.store[DISTANCE],
                                               self.store[COST],
                                               self.store[DURATION],)

    def __add__(self, other):
        self.store[TRUCKS] += other[TRUCKS]
        self.store[WORKERS] += other[WORKERS]
        self.store[DISTANCE] += other[DISTANCE]
        self.store[COST] += other[COST]
        self.store[DURATION] += other[DURATION]
        return self


class Colors:
    #HEADER = '\033[95m'
    #OKBLUE = '\033[94m'
    EMPH = '\033[93m'
    STRONG = '\033[92m'
    FAIL = '\033[91m'
    END = '\033[0m'

    def emph(text):
        """Print with emphasis."""
        print(Colors.EMPH + str(text) + Colors.END)

    def strong(text):
        """Print with strong emphasis."""
        print(Colors.STRONG + str(text) + Colors.END)


def aggregate(reader):
    """
    Reads a line of the given CSV reader and adds it to the aggregated results.

    The line is first converted to a Result object.

    Arguments:
    - `reader`: DictReader object
    """
    aggregated_results = {}
    for result_dict in reader:
        if (not result_dict[NAME].strip()):  # dict is empty
            continue
        result = Result(result_dict)
        name = result[NAME]
        aggregated_result = aggregated_results.get(name)
        if not aggregated_result:
            aggregated_results[name] = Aggregated_Result(result)
            continue
        else:
            aggregated_result.add(result)
    return aggregated_results


def check_bks(aggregated_results):
    reader = csv.DictReader(open(BKS_FILE), fieldnames=FIELDS)
    best_solutions = {}
    for result_dict in reader:
        result = Result(result_dict)
        best_solutions[result[NAME]] = result
    for key in best_solutions:
        if aggregated_results[key].best[COST] == best_solutions[key][COST]:
            Colors.emph("{name} {values}".format(name=key,
                values = aggregated_results[key].best))
        elif aggregated_results[key].best[COST] < best_solutions[key][COST]:
            Colors.strong("new best known solution:")
            Colors.strong("{name} {values}".format(name=key,
                values = aggregated_results[key].best))


def process_file(csv_file, args):
    """
    Takes an open csv_file and prints statistics about it.
    """
    print(csv_file.name)
    reader = csv.DictReader(csv_file, fieldnames=FIELDS)
    aggregated_results = aggregate(reader)
    check_bks(aggregated_results)
    if args.each:
        for key in sorted(aggregated_results):
            print(aggregated_results[key])
        print()
    if args.sums != 'none':
        keys = sorted(aggregated_results.keys())
    if (args.sums in ['all', 'best']):
        print("sum of best: ",
              sum([aggregated_results[k].best for k in keys], Result()))
    if (args.sums in ['all', 'average']):
        print("sum of avg:  ",
              sum([aggregated_results[k].average() for k in keys], Result()))
    if (args.sums in ['all', 'worst']):
        print("sum of worst:",
              sum([aggregated_results[k].worst for k in keys], Result()))
    print()


def main():
    """
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--sums', default='all',
                        choices=['all', 'average', 'best', 'none', 'worst'],
                        help='select which sums to print  (default: all)')
    parser.add_argument('-e', '--each', action="store_true",
                        help='print results for each instance')
    parser.add_argument('-l', '--latex', action="store_true",
                        help='print results as LaTeX table')
    parser.add_argument('-v', '--verbose', action="store_true",
                        help='print detailed results for each instance')
    parser.add_argument('infile', nargs='*', default=[],
                        help='one or more input files (default: stdin)')
    args = parser.parse_args()

    if args.verbose:
        args.each = True  # --verbose implies --each
        global VERBOSE
        VERBOSE = True

    if args.latex:
        global LATEX
        LATEX = True

    if not args.infile:
        print("no input file given, using stdin")
        process_file(sys.stdin, args)
    for infile in args.infile:
        csv_file = open(infile, 'r')
        process_file(csv_file, args)


if __name__ == '__main__':
    sys.exit(main())
