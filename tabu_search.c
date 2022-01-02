/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"
#include "config.h"
#include "local_search.h"
#include "node.h"
#include "problemreader.h"
#include "route.h"
#include "solution.h"
#include "vrptwms.h"
#include "wrappers.h"
#include "tabu_search.h"


///////////////////////////////////////////////////////////////////////////////
// File Scope (Static) Functions                                             //
///////////////////////////////////////////////////////////////////////////////

static void ts_construct_routes(Solution* sol, int workers);


//! Select and run a route construction heuristic for TS.
static void ts_construct_routes(Solution* sol, int workers) {
  switch (sol->pb->cfg->start_heuristic) {
    case SOLOMON:
      solve_solomon(sol, workers, sol->num_unrouted);
      return;
  }
  fprintf(stderr, "ERROR: start heuristic %s not available for TS.\n",
          START_HEURISTICS[sol->pb->cfg->start_heuristic]);
  fprintf(stderr, "pick any of\n%s\n",
          START_HEURISTICS[SOLOMON]);
  exit(EXIT_FAILURE);
}

///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////

//! "Constructor".
//! Allocate memory for a tabulist and initialize its members.
Tabulist* new_tabulist(Problem* pb) {
  Tabulist* tl = (Tabulist*) s_malloc(sizeof(Tabulist));
  tl->active = (pb->cfg->metaheuristic == TS);
  tl->iteration = 0;
  tl->nodes_routes_tabutime = (unsigned long) pb->cfg->tabutime;
  // this allocates more than strictly needed; we need one row per customer,
  // but allocate num_nodes rows which means that we allocate a row for the
  // depot as well; this is for reasons of safety, simplicity and performance
  // as it allows to index the rows with node ids (the depot has id 0, hence
  // the first ([0]) row will not be used).
  // The second dimension allocates num_nodes - 1 columns => one for each
  // route given that the worst case is that every node is on a separate
  // route. The route ids start with 0 and can hence be used directly.
  tl->nodes_routes = init_unsigned_long_matrix((size_t) pb->num_nodes,
                                               (size_t) pb->num_nodes - 1,
                                               tl->iteration);
  return tl;
}


//! "Destructor".
//! Free the memory of the given tabulist and all allocated members.
void free_tabulist(Tabulist* tl, size_t dim) {
  free_unsigned_long_matrix(tl->nodes_routes, dim);
  free(tl);
}


//! Return 1 if the given move is tabu, otherwise 0.
//! A move is considered tabu if any of the nodes in the move violate any
//! tabu criterion.
int is_move_tabu(Tabulist* tl, Move* m) {
  if (!tl->active) return 0;
  Node* n = m->first;
  do {
    if (tl->nodes_routes[n->id][m->target->id] > tl->iteration)
      return 1;
    n = n->next;
  } while (n != m->last->next);
  return 0;
}


//! Solve the given problem using the tabu search metaheuristic.
void solve_ts(Problem* pb, int workers) {
  // TODO: implement & remove 1
  // - implement move operator that allows increasing workers in order to
  // decrease trucks
  // - implement move operator that allows increasing trucks in order to
  // "shake" the solution
  fprintf(stderr, "WARNING: TS is not fully implemented yet\n");
  ts_construct_routes(pb->sol, workers);
  int state = REDUCE_TRUCKS;
  double best_cost = calc_costs(pb->sol, pb->cfg);
  Solution *sol = clone_solution(pb->sol);
  int updated = 0;
  Move m; init_move(&m, NON_IMPROVING);
  do {
    updated = 0;
    // TODO: hack to account for trucks and workers; needs to be replaced
    // by the content of the last todo; => remove 6
    if (pb->cfg->max_iterations &&
        (pb->tl->iteration * 2 > (unsigned long) pb->cfg->max_iterations))
      state = REDUCE_WORKERS;
    if (pb->cfg->runtime &&
        ((time((time_t*) NULL) - pb->start_time) * 2 > pb->cfg->runtime))
      state = REDUCE_WORKERS;
    for (int i = sol->trucks - 1; i >= 1; --i) {
      for (int j = i - 1; j >= 0; --j) {
        updated |= update_move(&m, sol->routes[j], sol->routes[i], state, 2);
        updated |= update_move(&m, sol->routes[i], sol->routes[j], state, 2);
        updated |= update_move(&m, sol->routes[j], sol->routes[i], state, 1);
        updated |= update_move(&m, sol->routes[i], sol->routes[j], state, 1);
      }
    }
    sol->workers_cache -= m.delta_workers;
    sol->dist_cache -= m.delta_dist;
    perform_move(sol, &m);
    sol->cost_cache = calc_cost(pb->cfg, sol->trucks, sol->workers_cache,
                                sol->dist_cache);
    if (sol->cost_cache < best_cost) {
      best_cost = sol->cost_cache;
      sol->time = time((time_t *)NULL) - pb->start_time;
      print_progress(sol);
      free_solution(pb->sol);
      pb->sol = clone_solution(sol);
    }
  } while (updated && proceed(pb, pb->tl->iteration));
  free_solution(sol);
}


//! Update the tabulist to consider the given move.
//! It is important that the iteration counter is updated at the beginning.
void update_tabulist_move(Tabulist* tl, Move* m) {
  if (!tl->active) return;
  Node* n = m->first;
  tl->iteration++;
  do {
    tl->nodes_routes[n->id][m->source->id] = tl->iteration +
                                             tl->nodes_routes_tabutime;
    n = n->next;
  } while (n != m->last->next);
}
