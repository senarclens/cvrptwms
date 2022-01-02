/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <cmath>
#include <iostream>
#include <time.h>

extern "C" {
  #include "common.h"
  #include "config.h"
  #include "grasp.h"
  #include "local_search.h"
  #include "problemreader.h"
  #include "solution.h"
  #include "vrptwms.h"
}

#include "cache.hpp"

#include "cached_grasp.hpp"



///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////


/**
 * Wrapper function to allow using solve_cached_aco from C.
 */
void c_solve_cached_grasp(Problem* pb, int workers)
{
  solve_cached_grasp(pb, workers);
}


/**
 * Solve the given problem with the ACO metaheuristic using a caching mechanism.
 *
 * This metaheuristic keeps track on how often a cache value has been hit.
 * This allows to react if the same cache value is hit over and over again by
 * resetting or shaking the pheromone data structure.
 */
void solve_cached_grasp(Problem* pb, int workers)
{
  std::cout << "WARNING: Implementation not finished yet!\n";  // TODO: remove
  Cache cache(*pb);
  double best_cost = INFINITY;
  double cost = INFINITY;
  unsigned long int hits = 0, max_hits = 5;  // TODO: make configurable
  Solution* sol = new_solution(pb);
  Solution* temp = NULL;
  while (proceed(pb, (unsigned long) pb->num_solutions)) {
    reset_solution(sol, pb->num_nodes);
    pb->num_solutions++;
    grasp_construct_routes(sol, workers);
    cost = calc_costs(sol, pb->cfg);
    hits = cache.contains(*sol);
    if (hits) {
//       if (hits > max_hits) {
//         if (pb->cfg->verbosity == DEBUG_CACHE) {
//           std::cout << "multiple HIT!! -> adjusting algorithm\n";
//         }
//       }
      continue;
    }
    cache.add(*sol);
    sol = do_ls(sol);
    cost = calc_costs(sol, pb->cfg);
    if (cost < best_cost) {
      best_cost = cost;
      sol->time = time((time_t *)NULL) - pb->start_time;
      print_progress(sol);
      temp = pb->sol;
      pb->sol = sol;
      sol = temp;
    }
  }
  std::cout << cache << "\n";  // TODO: remove
  free_solution(sol);
}
