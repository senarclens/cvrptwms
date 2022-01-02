/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

/*! \mainpage Solver for the VRPTWMS
 *
 * \section intro_sec Introduction
 *
 * This program allows solving vehicle routing problems with time windows and
 * multiple service workers. Currently, the only supported input file format
 * is the one used by Marius Solomon for his VRPTW instances.
 *
 * Because of the close relationship between the VRPTWMS and the VRPTW (which
 * is basically a VRPTWMS limited to one service worker), the solver also
 * allows to solve the more generic VRPTW instances. This is very useful
 * in order to determine the solver's quality and performance.
 *
 * The solver is a collection of route construction heuristics, metaheuristics
 * and local search operators which can be combined at will. The selection
 * of the desired components can be done in the configuration file
 * (<code>vrptwms.conf</code>) or via commandline parameters.
 *
 * \section install_sec Installation
 * Please see the included README file for installation instructions.
 *
 * \section execution Execution
 * After the program was correctly installed and compiled it can be run using
 * <code>./vrptwms</code>. To get help on the available options, use
 * <pre>./vrptwms -h</pre>
 *
 * \subsection step1 Sample Run
 * A simple run using the ant colony optimazation metaheuristic running for
 * 20 seconds per problem instance can be started with
 * <pre>./vrptwms -m aco -r 20 data/R1??.txt</pre>
 *
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ant_colony_optimization.h"
#include "common.h"
#include "config.h"
#include "grasp.h"
#include "local_search.h"
#include "node.h"
#include "problemreader.h"
#include "route.h"
#include "solution.h"
#include "stats.h"
#include "tabu_search.h"
#include "wrappers.h"
#include "vns.h"
#include "vrptwms.h"

extern void c_solve_cached_aco(Problem* pb, int workers);
extern void c_solve_cached_grasp(Problem* pb, int workers);

///////////////////////////////////////////////////////////////////////////////
// File Scope (Static) Functions                                             //
///////////////////////////////////////////////////////////////////////////////
static Node* get_best_seed(Node* unrouted, double **c_m);

//! Return the best sequential seed (which is the furthest from the depot).
//! \return best seed or NULL if there are no more candidates available
static Node* get_best_seed(Node* unrouted, double **c_m) {
  Node *best_cand = NULL;
  double max_dist = -1.0;
  while (unrouted) {
    if (c_m[0][unrouted->id] > max_dist) {
      max_dist = c_m[0][unrouted->id];
      best_cand = unrouted;
    }
    unrouted = unrouted->next;
  }
  return best_cand;
}


///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////

//! Add a VRPTWMS instance's result to the list of best results.
Resultlist* add_result(Problem* pb) {
  int workers = 0;
  double dist = 0.0;
  Resultlist* result = (Resultlist*) s_malloc(sizeof(Resultlist));
  result->name = get_name(pb->name);
  result->trucks = pb->sol->trucks;
  result->time = pb->sol->time;
  result->saturation_time = pb->sol->saturation_time;
  for (int i=0; i<pb->sol->trucks; i++) {
    workers += pb->sol->routes[i]->workers;
    dist += calc_length(pb->sol->routes[i]);
  }
  result->workers = workers;
  result->distance = dist;
  result->cost = calc_costs(pb->sol, pb->cfg);
  result->next = NULL;
  return result;
}


//! Free the memory of a given result list.
void free_results(Resultlist *results) {
  Resultlist *temp = (Resultlist *) NULL;
  while (results) {
    free(results->name);
    temp = results->next;
    free(results);
    results = temp;
  }
}


//! Return one of the most promising seed nodes for sequential construction.
//! The quality of the seed is determined by a roulette wheel selection
//! of a candidate's seed pheromone and its distance from the depot.
//! Increasing distance and higher pheromone are better. For
//! non-pheromone based heuristics, all pheromone values are simply set to 1.
//! Note: only the pheromone between the current route's depot and the
//! potential seed is taken into account - not the pheromone between the seed
//! and all depots. This may be considered a feature or an issue.
Node* get_seed(Solution* sol) {
  double *d = sol->pb->c_m[0][DEPOT];  // dist. from depot
  Node *nl = sol->unrouted;
  double cum_attractiveness = 0.0;
  double **p_m = sol->pb->pheromone;
  double trail[sol->num_unrouted];
  double* trail_ptr = trail;
  int num_nodes = sol->pb->num_nodes;
#ifdef DEBUG
  if (sol->pb->cfg->verbosity >= FULL_DEBUG)
    printf("seed selection\n");
#endif
  while (nl) {
    // trail denominator is the same for all nodes => no div. needed
    (*trail_ptr) = (p_m[num_nodes + sol->trucks][nl->id] +
                    p_m[nl->id][num_nodes + sol->trucks]);
    cum_attractiveness += d[nl->id] * (*trail_ptr);
    ++trail_ptr;
    nl = nl->next;
  }
  nl = sol->unrouted;
  trail_ptr -= sol->num_unrouted;
  double threshold = drand48() * cum_attractiveness;
  while (nl) {
    cum_attractiveness -= d[nl->id] * (*trail_ptr);
    if (threshold >= cum_attractiveness) {
#ifdef DEBUG
      if (sol->pb->cfg->verbosity >= FULL_DEBUG)
        printf("new route's seed: %d\n", nl->id);
#endif
      return nl;
    }
    trail_ptr++;
    nl = nl->next;
  }
  fprintf(stderr, "ERROR: get_seed: no seed selected\n");
  return (Node *) NULL;
}


//! Print a performance summary.
//! The summary is only printed for adequate verbosity levels.
void fprint_performance(FILE* stream, Problem* pb) {
  if (((pb->cfg->verbosity < BASIC_VERBOSITY) && (stream == stdout)) ||
      !pb->cfg->metaheuristic)
    return;
  unsigned long iterations = pb->tl->active ? pb->tl->iteration :
    (unsigned long) pb->num_solutions;
  time_t duration = time((time_t*) NULL) - pb->start_time;
  duration = duration ? duration : 1;
  fprintf(stream, "calculated %lu iterations/s\n",
          iterations / (unsigned long) duration);
}


//! Print summary of the current best solution.
//! Note that the cost caches are not recalculated and have to be up to date.
void print_progress(Solution* sol) {
  if (sol->pb->cfg->verbosity >= BASIC_DEBUG)
    printf("%d %d %f -> %f (%ld seconds)\n", sol->trucks, sol->workers_cache,
            sol->dist_cache, sol->cost_cache, sol->time);
}


//! Print an aggregated output of all processed instances.
void print_results(Resultlist *results, Config *cfg) {
  if (!results) return;
  int sum_trucks = 0; int sum_workers = 0; double sum_distance = 0.0;
  double sum_cost = 0.0; long int sum_time = 0; int cnt = 0;
  char col1_4[] = "|------------+--------+---------+----------";
  char col5_6[] = "+------------+----------|";
  if (cfg->format == CSV) {
    if (cfg->verbosity) // BASIC_VERBOSITY
      printf("name, trucks, workers, distance, cost, time [s]\n");
    while (results) {
    if (cfg->metaheuristic) {
      printf("%s,%d,%d,%.2f,%.6f,%ld",
           results->name, results->trucks, results->workers,
           results->distance, results->cost, results->time);
    } else {
      printf("%s,%d,%d,%.2f,%.6f,%s",
           results->name, results->trucks, results->workers,
           results->distance, results->cost, "n/a");
    }
    if (results->saturation_time) {
      printf(",%ld", results->saturation_time);
    }
    printf("\n");
    results = results->next;
    }
    return;
  }
  printf("%s%s\n", col1_4, col5_6);
  printf("| name       | trucks | workers | distance |  cost      |");
  printf(" time [s] |\n");
  printf("%s%s\n", col1_4, col5_6);
  while (results) {
    if (cfg->metaheuristic)
      printf("| %10s | %6d | %7d | %8.2f | %10.6f | %8ld |\n",
           results->name, results->trucks, results->workers,
           results->distance, results->cost, results->time);
    else
      printf("| %10s | %6d | %7d | %8.2f | %10.6f | %8s |\n",
           results->name, results->trucks, results->workers,
           results->distance, results->cost, "n/a");
    sum_trucks += results->trucks;
    sum_workers += results->workers;
    sum_distance += results->distance;
    sum_cost += results->cost;
    sum_time += results->time;
    cnt++;
    results = results->next;
  }
  printf("%s%s\n", col1_4, col5_6);
  if (cnt > 1) {
    printf("| %10s | %6d | %7d | %8.2f | %10.6f | %8ld |\n", "sum",
         sum_trucks, sum_workers, sum_distance, sum_cost, sum_time);
    printf("| %10s | %6.2f | %7.2f | %8.2f | %10.6f | %8.2f |\n", "avg",
         (double) sum_trucks / cnt, (double) sum_workers / cnt,
         sum_distance / cnt, sum_cost / cnt, (double) sum_time / cnt);
    printf("%s%s\n", col1_4, col5_6);
  }
}


//! Return true if the solver should keep running.
//! Neither the maximum runtime nor the max. number of iterations is allowed
//! to be reached.
int proceed(Problem* pb, unsigned long iteration) {
  int timeout = ((pb->cfg->runtime) &&
                 (time((time_t*) NULL) - pb->start_time >= pb->cfg->runtime));
  int runsout = ((pb->cfg->max_iterations) &&
                 (iteration >= (unsigned long) pb->cfg->max_iterations));
  return (timeout || runsout) ? 0 : 1;
}


int solve(Problem *pb, int workers, int fleetsize) {
  switch (pb->cfg->metaheuristic) {
  case ACO:
    solve_aco(pb, workers);
    break;
  case CACHED_ACO:
    c_solve_cached_aco(pb, workers);
    break;
  case CACHED_GRASP:
    c_solve_cached_grasp(pb, workers);
    break;
  case GACO:
    solve_gaco(pb, workers);
    break;
  case GRASP:
    solve_grasp(pb, workers);
    break;
  case TS:
    solve_ts(pb, workers);
    break;
  case VNS:
    solve_vns(pb, workers);
    break;
  case NO_METAHEURISTIC:
    solve_solomon(pb->sol, workers, fleetsize);
    pb->sol = do_ls(pb->sol);
    break;
  default:  // no metaheuristic
    fprintf(stderr, "ERROR: metaheuristic %s not available.\n",
          METAHEURISTICS[pb->cfg->metaheuristic]);
    exit(EXIT_FAILURE);
    break;
  }
  return 0;
}


//! Construct a single initial solution.
//! Use either a deterministic or a stochastic version of Solomon's I1
//! heuristic.
int solve_solomon(Solution* sol, int workers, int fleetsize) {
  Problem* pb = sol->pb;
  Node *unrouted = (Node *) NULL;
  Route *route = (Route *) NULL;
  Insertion ins = {route, NULL, NULL, INFINITY, 0.0, NULL, NULL};
  Insertion insertions[sol->num_unrouted];
  double min_cost =INFINITY;
  if (!pb->cfg->deterministic) {
    for (int i = 0; i < sol->num_unrouted; ++i) {
      insertions[i].target = route;
      insertions[i].node = (Node *) NULL;
      insertions[i].after = (Node *) NULL;
      insertions[i].cost = INFINITY;
      insertions[i].attractiveness = 0.0;
      insertions[i].next = (Insertion *) NULL;
      insertions[i].prev = (Insertion *) NULL;
    }
  }
  if (pb->cfg->verbosity >= BASIC_DEBUG) {
    printf ("depot: ");
    print_node(pb->nodes[0]);
  }
  while (sol->unrouted) {
    if (sol->trucks == fleetsize) return sol->num_unrouted;
    if (pb->cfg->deterministic)
      unrouted = get_best_seed(sol->unrouted, pb->c_m[0]);
    else
      unrouted = get_seed(sol);
    #ifdef DEBUG
    if (pb->cfg->verbosity >= BASIC_DEBUG) {
      printf ("seed: "); print_node(unrouted);
    }
    #endif // DEBUG
    remove_unrouted(sol, unrouted);
    route = new_route(sol, unrouted, workers);
    while (sol->unrouted) { // fill the current route
      ins.cost = INFINITY;
      unrouted = sol->unrouted;
      if (pb->cfg->deterministic) {
        while (unrouted) {
          calc_best_insertion(route, unrouted, &ins);
          unrouted = unrouted->next;
        }
        if (isinf(ins.cost))
          break;
      } else {
        min_cost = INFINITY;
        for (int i = 0; i < sol->num_unrouted; ++i) {
          insertions[i].cost = INFINITY; // reset rel. part of ins
          calc_best_insertion(route, unrouted, &insertions[i]);
          min_cost = fmin(min_cost, insertions[i].cost);
          unrouted = unrouted->next;
        }
        if (isinf(min_cost))
          break;
        // TODO: change this to use public function from route.{c,h}
        ins = *aco_pick_insertion(insertions, sol->num_unrouted, min_cost);
      }
      #ifdef DEBUG
      if (pb->cfg->verbosity >= BASIC_DEBUG) {
        printf("adding: "); print_node(ins.node);
      }
      #endif // DEBUG
      remove_unrouted(sol, ins.node);
      add_nodes(ins.target, ins.node, ins.node, ins.after);
    }
  }
  return 0;
}


