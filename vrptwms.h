/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef VRPTWMS_H
#define VRPTWMS_H

#include <stdio.h>

#include "common.h"



//! \struct resultlist
//! Singly-linked list to store all solutions' objective function values.
//! If the solver deals with more than one input file, this struct stores
//! the already obtained results. Not that the solutions themselves are not
//! stored, just their benchmark data.
struct resultlist {
  char* name;  //!< the input file's name without its extension
  int trucks;  //!< the number of trucks (routes) used by the solution
  int workers;  //!< the number of workers used by the solution
  double distance;  //!< the required total distance
  double cost;  //!< the objective function's value
  long int time;  //!< processing time in seconds to obtain this solution
  long int saturation_time;  //!< # seconds until the cache saturated or 0
  Resultlist* next;  //!< the next element or (Resultlist*) NULL
};


Resultlist* add_result(Problem*);
void fprint_performance(FILE* stream, Problem*);
void free_results(Resultlist*);
Node* get_seed(Solution* sol);
void print_progress(Solution*);
void print_results(Resultlist*, Config*);
int proceed(Problem*, unsigned long count);
int solve(Problem*, int workers, int fleetsize);
int solve_solomon(Solution*, int workers, int fleetsize);

#endif
