/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef PROBLEMREADER_H
#define PROBLEMREADER_H

#include <stdio.h>
#include <time.h>

#include "common.h"

enum problem_state {
  REDUCE_TRUCKS,
  REDUCE_WORKERS,
  REDUCE_DISTANCE
};

struct problem {
  long int attempts;  // remaining reduction attempts in the current state
  unsigned int capacity;  //!< the truck's capacity
  Config* cfg;
  //! Array of cost matrices.
  //! [0] for distances, [n] includes servicetime for n workers
  double*** c_m;
  long num_solutions;  //!< counts the total iterations
  char* name;  //!< the input file's name without its extension
  Node** nodes;  //!< array of Node* including the depot
  int num_nodes;  //!< number of nodes including the depot
  double** pheromone;  //!< pheromone matrix, initialized to [1 .. 1]
  Solution* sol;  //!< pointer to the currently best solution
  time_t start_time;
  enum problem_state state;
  Tabulist* tl;  //!< Different tabu criteria (mainly for TS).
  Stats* stats;  //!< For collecting statistical data for TS.
};

void free_problem(Problem*);
char* get_name(const char* fname);
Problem *get_problem(const char* fname, Config* cfg_ptr);
Node *new_depot(Problem*);
void print_problem(Problem*);

#endif
