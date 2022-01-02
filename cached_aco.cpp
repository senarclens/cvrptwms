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
  #include "ant_colony_optimization.h"
  #include "common.h"
  #include "config.h"
  #include "local_search.h"
  #include "problemreader.h"
  #include "solution.h"
  #include "vrptwms.h"
}

#include "cache.hpp"

#include "cached_aco.hpp"



///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////


/**
 * Wrapper function to allow using solve_cached_aco from C.
 */
extern "C" void c_solve_cached_aco(Problem* pb, int workers)
{
  solve_cached_aco(pb, workers);
}


/**
 * Reset the pheromone matrix to its configured initial values.
 *
 * The matrix' first row and column are ignored throughout the program and
 * hence not updated by this function either.
 */
static void reset_pheromone(Problem* pb) {
  double** p_m = pb->pheromone;
  for (int i = 1; i < (2 * pb->num_nodes - 1); ++i) {  // ignore 0 DEPOT
    for (int j = 1; j < (2 * pb->num_nodes - 1); ++j) {
      p_m[i][j] = pb->cfg->initial_pheromone;
    }
  }
  #ifdef DEBUG
  if (pb->cfg->verbosity == DEBUG_CACHE) {
    printf("resetting pheromone...\n");
  }
  #endif
}


/**
 * Reset the pheromone to random values [min_pheromone, 1.0).
 *
 * The matrix' first row and column are ignored throughout the program and
 * hence not updated by this function either.
 */
static void shake_pheromone(Problem* pb) {
  double** p_m = pb->pheromone;
  double new_pheromone = 1.0, min_pheromone = pb->cfg->min_pheromone;
  for (int i = 1; i < (2 * pb->num_nodes - 1); ++i) {  // ignore 0 DEPOT
    for (int j = 1; j < (2 * pb->num_nodes - 1); ++j) {
      new_pheromone = drand48();
      p_m[i][j] = max(new_pheromone, min_pheromone);
    }
  }
  #ifdef DEBUG
  if (pb->cfg->verbosity == DEBUG_CACHE) {
    printf("shaking pheromone to\n");
    print_double_matrix(2 * pb->num_nodes - 1, pb->pheromone, "pheromone");
  }
  #endif
}


/**
 * Solve the given problem with the ACO metaheuristic using a caching mechanism.
 *
 * This metaheuristic keeps track on how often a cache value has been hit.
 * This allows to react if the same cache value is hit over and over again by
 * resetting or shaking the pheromone data structure.
 */
void solve_cached_aco(Problem* pb, int workers)
{
  double best_cost = INFINITY;
  double cost = INFINITY;
  Solution* sol = new_solution(pb);
  Solution* temp = NULL;
  Cache cache(*pb);
  unsigned long int hits = 0;
  unsigned long int max_hits = 5;  // TODO: make configurable
  bool saturized = false;  // to measure if speedups can be gained
  while (proceed(pb, (unsigned long) pb->num_solutions)) {
    for (int i = 0; i < pb->cfg->ants; ++i) {  // solve once for each ant
      reset_solution(sol, pb->num_nodes);
      aco_construct_routes(sol, workers);

      cost = calc_costs(sol, pb->cfg);  // required for cache; TODO: refactor!!!
      hits = cache.contains(*sol);
      if (hits) {
        if (hits > max_hits and !saturized) {
          saturized = true;
          pb->sol->saturation_time = time((time_t*)NULL) - pb->start_time;
//           if (pb->cfg->verbosity == DEBUG_CACHE) {
//             std::cout << "HIT!! -> resetting pheromone\n";
//           }
//           shake_pheromone(pb);
//           reset_pheromone(pb);  // TODO: maybe skip and only tweak parameters
//           pb->cfg->alpha = drand48();  // TODO: maybe try range(0.9, 0.0, -0.1)
//           max_hits += 2;  // TODO: make configurable or remove (after testing)
        }
        continue;
      }
      cache.add(*sol);

      sol = do_ls(sol);
      cost = calc_costs(sol, pb->cfg);
      if (cost < best_cost) {
        best_cost = cost;
        sol->time = time((time_t*)NULL) - pb->start_time;
        print_progress(sol);
        temp = pb->sol;
        pb->sol = sol;
        sol = temp;
      }
    }
    pb->num_solutions += pb->cfg->ants;
    update_pheromone(pb, pb->sol);
  }

  //   std::cout << cache << "\n";  // TODO: remove
  free_solution(sol);
}

