/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <stdlib.h>

#include "common.h"
#include "config.h"
#include "local_search.h"
#include "node.h"
#include "problemreader.h"
#include "route.h"
#include "solution.h"
#include "vrptwms.h"
#include "vns.h"


///////////////////////////////////////////////////////////////////////////////
// File Scope (Static) Functions                                             //
///////////////////////////////////////////////////////////////////////////////

static void vnc_construct_routes(Solution* sol, int workers);


//! Select and run a route construction heuristic for ACO.
static void vnc_construct_routes(Solution* sol, int workers) {
  solve_solomon(sol, workers, sol->num_unrouted);
}


//! Attempt to move all nodes from a route and return the number of moved nodes.
//! This function usually worsens a solution and should only be used to leave
//! a local optimum.
static int distribute_nodes(Solution* sol, int route_idx) {
  Route *source = sol->routes[route_idx];
  int old_len = source->len;
  Node *n = source->nodes->next;
  Insertion ins = {NULL, NULL, NULL, INFINITY, 0.0, NULL, NULL};
  if (source->len == EMPTY)
    return 0;
  while (n != source->tail) {
    ins.node = n;
    ins.target = (Route *) NULL;
    ins.cost = INFINITY;
    ins.attractiveness = 0.0;
    ins.after = (Node *) NULL;
    for (int j = 0; j < sol->trucks; ++j) {
      if (j == route_idx) {
        continue; // don't move nodes from a route to itself
      }
      calc_best_insertion(sol->routes[j], n, &ins);
    }
    n = n->next;
    if (isinf(ins.cost))
      continue;
    remove_nodes(source, ins.node, ins.node);
    add_nodes(ins.target, ins.node, ins.node, ins.after);
    if (source->len == EMPTY) {
      remove_route(sol, route_idx);
      // TODO: remove 1
//       printf("moved %d nodes during shake\n", (old_len - EMPTY));
      return old_len - EMPTY;
    }
  }
  // TODO: remove 1
//   printf("moved %d nodes during shake\n", (old_len - source->len));
  return old_len - source->len;
}


//! Improve the given solution doing a deterministic local search.
static void improve_solution(Solution* sol) {
  int improved = 0;

  // reduce trucks
  do {
    improved = move_all(sol, REDUCE_TRUCKS);
    improved |= swap_all(sol);
  } while (improved);

  // reduce workers
  for (int i = 0; i < sol->trucks; ++i) {
    reduce_service_workers(sol->routes[i]);
  }
  do {
    improved = move_all(sol, REDUCE_WORKERS);
    improved |= swap_all(sol);
  } while (improved);
}


//! Return a solution after it was shaked away from a local optimum.
//! It is attempted to make the new solution rather diverse.
//! To give a good amount of freedom to the shake and subsequent local search,
//! all routes are reset to the maximum number of workers.
static void shake_solution(Solution* sol) {
  for (int i = 0; i < sol->trucks; ++i) {
    sol->routes[i]->workers = (int) sol->pb->cfg->max_workers;
  }
  // skew is not a practical issue as sol->trucks is much smaller than 2^31
  // see http://stackoverflow.com/a/2999130/104659
  int route_index = (int) (lrand48() % sol->trucks);
  // TODO: the loop below is potentially infinite (in the unlikely event that
  // no possible moves are left); catch this and allow for another shake
  // TODO: rem 1
//   print_route(stdout, sol->routes[route_index]);
  while(!distribute_nodes(sol, route_index)) {
    route_index = (int) (lrand48() % sol->trucks);
  }
  // TODO: rem 1
//   print_route(stdout, sol->routes[route_index]);
//   for (int i = sol->trucks - 1; i >= 0; --i) {
//     distribute_nodes(sol, i);
//   }
}

///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////

//! Solve the given problem using the tabu search metaheuristic.
void solve_vns(Problem* pb, int workers) {
  // TODO: implement & remove 1
  fprintf(stderr, "WARNING: VNS is not fully implemented yet\n");
  vnc_construct_routes(pb->sol, workers);
  pb->sol = do_ls(pb->sol);
  double cost = calc_costs(pb->sol, pb->cfg);
  double best_cost = cost;
  Solution* sol = clone_solution(pb->sol);
  while (proceed(pb, (unsigned long) pb->num_solutions)) {
    shake_solution(sol);
    improve_solution(sol);
    // TODO: remove
//     sol = do_ls(sol);
    cost = calc_costs(sol, pb->cfg);
//     printf("cost: %f\n", cost);
    if (cost < best_cost) {
// TODO: remove 1
//       printf("updating solution\n");
      best_cost = cost;
      sol->time = time((time_t *)NULL) - pb->start_time;
      print_progress(sol);
      free_solution(pb->sol);
      pb->sol = clone_solution(sol);
    }
    pb->num_solutions++;
  }
  free_solution(sol);
}

