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

#include "grasp.h"


///////////////////////////////////////////////////////////////////////////////
// File Scope (Static) Functions                                             //
///////////////////////////////////////////////////////////////////////////////

static void grasp_solve_solomon(Solution* sol, int workers);


//! Create an initial solution using Solomon's I1 heuristic.
//! The heuristic has been adapted for the GRASP metaheuristic.
static void grasp_solve_solomon(Solution* sol, int workers) {
  Insertion_List il; init_insertion_list(&il, sol->pb->cfg->rcl_size);
  Insertion* ins = (Insertion*) NULL;
  while (sol->unrouted) {
    Node *unrouted = get_seed(sol);
    remove_unrouted(sol, unrouted);
    Route* route = new_route(sol, unrouted, workers);
    while (sol->unrouted) {  // fill the current route
      unrouted = sol->unrouted;
      while (unrouted) {
        ins = get_best_insertion(route, unrouted);
        if (ins)
          update_insertion_list(&il, ins);
        unrouted = unrouted->next;
      }
      ins = pick_insertion(&il, sol->pb->cfg->use_weights);
      if (!ins) break;
      remove_unrouted(sol, ins->node);
      add_nodes(ins->target, ins->node, ins->node, ins->after);
      reset_insertion_list(&il);
    }
  }
}


///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////

//! Select and run a route construction heuristic for ACO.
void grasp_construct_routes(Solution* sol, int workers) {
  switch (sol->pb->cfg->start_heuristic) {
    case SOLOMON:
      grasp_solve_solomon(sol, workers);
      return;
  }
  fprintf(stderr, "ERROR: start heuristic %s not available for GRASP.\n",
          START_HEURISTICS[sol->pb->cfg->start_heuristic]);
  fprintf(stderr, "pick any of\n%s\n",
          START_HEURISTICS[SOLOMON]);
}


//! Solve the given problem using the GRASP metaheuristic.
void solve_grasp(Problem* pb, int workers) {
  double best_cost = INFINITY;
  double cost = INFINITY;
  Solution* sol = new_solution(pb);
  Solution* temp = NULL;
  while (proceed(pb, (unsigned long) pb->num_solutions)) {
    grasp_construct_routes(sol, workers);
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
    reset_solution(sol, pb->num_nodes);
    pb->num_solutions++;
  }
  free_solution(sol);
}

