/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef SOLVER_H
#define SOLVER_H

#include <stdio.h>
#include "common.h"
#include "config.h"

//! \struct solution
//! Stores a single solution to a VRPTWMS.
//! The solution starts unsolved with all nodes in the list of unrouted nodes.
//! A feasible solution must therefore not have any unrouted nodes.
//! The cost, distance and workers are not permanently updated (for
//! performance reasons) and may hence contain outdated values.
struct solution {
    Route** routes;  //!< array of pointers to the solution's routes
    int trucks;  //!< the number of trucks (routes) used by the solution
    Node* unrouted;  //!< double linked list of pointers to unrouted nodes
    int num_unrouted;
    long int time;  //!< processing time in seconds to obtain this solution
    long int saturation_time;  //!< # seconds until the cache saturated or 0
    int workers_cache;  //!< The total number of workers required.
    double dist_cache;  //!< The total distance required by this solution.
    double cost_cache;  //!< The total cost of this solution.
    Problem* pb;  //!< Pointer to the problem instance.
};

Solution* new_solution(Problem*);
void assert_feasibility(Solution*);
double calc_costs(Solution* sol, const Config* cfg);
double calc_dist(Solution*);
int calc_workers(Solution*);
Solution* clone_solution(Solution*);
void fprint_solution(FILE* stream, Solution*, Config*, int verbose);
void free_solution(Solution*);
int get_route_index(Solution*, int route_id);
void remove_route(Solution*, int route_idx);
void remove_unrouted(Solution*, Node *node);
void reset_solution(Solution*, int num_nodes);
void save_solution_details(Solution*, Config*);


//! Calculate the cost of a number of trucks, workers and a given distance.
static inline double calc_cost(const Config* cfg, int trucks, int workers,
                               double distance) {
  return (distance * cfg->cost_distance + workers * cfg->cost_worker +
          trucks * cfg->cost_truck);
}

//! Swap the solutions pointed to by the arguments.
static inline void swap_solution(Solution** first, Solution** second) {
  Solution* temp = *first;
  *first = *second;
  *second = temp;
}

#endif
