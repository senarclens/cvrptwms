/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "node.h"
#include "problemreader.h"
#include "route.h"
#include "wrappers.h"
#include "vrptwms.h"
#include "solution.h"

///////////////////////////////////////////////////////////////////////////////
// File Scope (Static) Functions                                             //
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////

//! "Constructor".
Solution* new_solution(Problem *pb) {
  Solution *sol = (Solution *) s_malloc(sizeof(Solution));
  sol->pb = pb;
  int num_nodes = sol->pb->num_nodes;
  Node **nodes = sol->pb->nodes;
  Node *tail = (Node *) NULL;
  sol->num_unrouted = num_nodes - 1;
  // allocate space for pointers to one route per node
  sol->routes = (Route**) s_malloc(sizeof(Route*) *
  (size_t) (num_nodes - 1));  // exclude the depot
  sol->trucks = 0;
  sol->time = 0;
  sol->saturation_time = 0;
  sol->workers_cache = 0;
  sol->dist_cache = 0.0;
  sol->unrouted = clone_node(nodes[1]);
  sol->unrouted->prev = (Node*) NULL;
  sol->unrouted->next = (Node*) NULL;
  tail = sol->unrouted;
  for (int i = 2; i < num_nodes; i++) { // skip depot and first node
    tail->next = clone_node(nodes[i]);
    tail->next->prev = tail;
    tail->next->next = (Node*) NULL;
    tail = tail->next;
  }
  return sol;
}


//! Interrupt unless the solution is feasible.
//! Feasibility implies that all routes are feasible and all customers are
//! served exactly once.
void assert_feasibility(Solution *sol) {
  int feasible = 1;
  Node *n = (Node *) NULL;
  int num_nodes = sol->routes[0]->pb->num_nodes; // includes the depot
  if (!sol->routes[0]) {
    printf("sol_is_feasible: no routes in solution");
    exit(EXIT_FAILURE);
  }
  char served[num_nodes];
  Node **nodes = sol->routes[0]->pb->nodes;
  served[0] = 1;  // no need to serve the depot
  for (int i = 1; i < num_nodes; ++i) {
    served[i] = 0;
  }
  for (int i = 0; i < sol->trucks; ++i) {
    if (!is_feasible(sol->routes[i]))
      feasible = 0;
    n = sol->routes[i]->nodes->next;
    while (n->next) {  // until n is the closing depot
      served[n->id]++;
      n = n->next;
    }
  }
  for (int i = 0; i < num_nodes; ++i) {
    if (served[i] > 1) {
      print_node(nodes[i]);
      fprintf(stderr, "was served more than once\n");
      feasible = 0;
    } else if (served[i] < 1) {
      print_node(nodes[i]);
      fprintf(stderr, "was not served at all\n");
      feasible = 0;
    }
  }
  if (!feasible) {
    fprint_solution(stderr, sol, sol->pb->cfg, 1);
    fprintf(stderr, "ERROR: solution is not feasible; exiting...\n");
    exit(EXIT_FAILURE);
  }
}


//! Return the objective function's value.
double calc_costs(Solution* sol, const Config* cfg) {
  int workers = 0;
  double dist = 0.0;
  for (int i=0; i<sol->trucks; i++) {
    workers += sol->routes[i]->workers;
    dist += calc_length(sol->routes[i]);
  }
  // TODO: remove during refactoring
  sol->workers_cache = workers;
  sol->dist_cache = dist;
  sol->cost_cache = calc_cost(cfg, sol->trucks, workers, dist);
  return sol->cost_cache;
}


//! Return the total distance required by this solution.
double calc_dist(Solution* sol)
{
  double dist = 0.0;
  for (int i=0; i<sol->trucks; i++) {
    dist += calc_length(sol->routes[i]);
  }
  return dist;
}


//! Return the total number of workers required by this solution.
int calc_workers(Solution* sol)
{
  int workers = 0;
  for (int i=0; i<sol->trucks; i++) {
    workers += sol->routes[i]->workers;
  }
  return workers;
}


//! Clone a solution and return a pointer to the clone.
//! The clone gets entirely new route objects.
Solution* clone_solution(Solution* sol) {
  Solution* clone = (Solution*) s_malloc(sizeof(Solution));
  int num_nodes = sol->pb->num_nodes;
  clone->pb = sol->pb;
  clone->num_unrouted = sol->num_unrouted;
  clone->trucks = sol->trucks;
  clone->time = sol->time;
  clone->saturation_time = sol->saturation_time;
  clone->cost_cache = sol->cost_cache;
  clone->dist_cache = sol->dist_cache;
  clone->workers_cache = sol->workers_cache;
  // don't do sizeof(Route *) * clone->trucks b/c the solution
  // might be reset and use more trucks in a future run
  // sizeof(Route *) * clone->num_unrouted doesn't work b/c
  // num_unrouted is generally 0 after running the initial heuristic
  clone->routes = (Route **) s_malloc(sizeof(Route *) * (size_t) num_nodes);
  for (int i = 0; i < clone->trucks; ++i) {
    clone->routes[i] = clone_route(sol->routes[i]);
  }
  clone->unrouted = (Node *) NULL;
  if (sol->unrouted) {
    Node* n = sol->unrouted;
    clone->unrouted = clone_node(n);
    Node* tail = clone->unrouted;
    n = n->next;
    while (n) {
      tail->next = clone_node(n);
      tail->next->prev = tail;
      tail = tail->next;
      n = n->next;
    }
  }
  return clone;
}


//! Write a representation of the solution to the given filestream.
void fprint_solution(FILE* stream, Solution* sol, Config* cfg, int verbose) {
  if (verbose) {
    fprintf(stream, "%s\n", sol->pb->name);
    if (stream != stdout)
      fprint_config_summary(stream, cfg);
    fprint_performance(stream, sol->pb);
    fprintf(stream, "found best solution after %ld seconds\n", sol->time);
    for (int i=0; i<sol->trucks; i++)
      print_route(stream, sol->routes[i]);
  }
  calc_costs(sol, sol->pb->cfg);
  fprintf(stream, "trucks: %d, workers: %d, distance: %.2f, cost: %.6f\n",
          sol->trucks, sol->workers_cache, sol->dist_cache, sol->cost_cache);
}


//! Free the memory of the given solution and all allocated members.
void free_solution(Solution* sol) {
  Node *n = (Node *) NULL;
#ifdef DEBUG
  if (sol->pb->cfg->verbosity >= FULL_DEBUG)
    printf("free_solution: trying to free %d routes\n", sol->trucks);
#endif // DEBUG
  for (int i = 0; i < sol->trucks; ++i) {
    free_route(sol->routes[i]);
    sol->routes[i] = (Route *) NULL;
  }
  free(sol->routes);
  while (sol->unrouted) {
    n = sol->unrouted;
    remove_unrouted(sol, sol->unrouted);
    free(n);
  }
  free(sol);
}


//! Return the index of the given route in the solution's route array.
int get_route_index(Solution* sol, int route_id) {
  for (int i = 0; i < sol->trucks; ++i) {
    if (sol->routes[i]->id == route_id)
      return i;
  }
  fprintf(stderr, "ERROR: get_route_index: route_id not found\n");
  exit(EXIT_FAILURE);
}


//! Remove a route from the solution and free its memory.
//! Only use on empty routes (routes with only the depot).
void remove_route(Solution* sol, int route_idx) {
  if (!(sol->routes[route_idx]->len == EMPTY)) {  // non-empty route
    fprintf(stderr, "remove_route tried to remove non-empty route\n");
    exit(EXIT_FAILURE);
  }
  free_route(sol->routes[route_idx]);
  sol->trucks--;
  for (int i = route_idx; i < sol->trucks; ++i) {
    sol->routes[i] = sol->routes[i+1];
  }
  sol->routes[sol->trucks] = (Route *) NULL;
}


// Remove an unrouted node from the solution's list of unrouted nodes.
// Needs to be called before a node is added to a route, otherwise this
// will remove the node from the route
void remove_unrouted(Solution* sol, Node* nl) {
#ifdef DEBUG
  if (!nl) {
    printf("remove_unrouted: nl: NULL\n");
    exit(EXIT_FAILURE);
  }
#endif // DEBUG
  if (nl->prev) {
    nl->prev->next = nl->next;
  } else {
    sol->unrouted = nl->next;
  }
  if (nl->next) {
    nl->next->prev = nl->prev;
  }
  sol->num_unrouted--;  // TODO: maybe remove counting unrouted in general
}


//! Reset the given solution to the state it had after it was initialized.
//! Prepend each route's nodes to the unrouted nodes.
void reset_solution(Solution* sol, int num_nodes) {
  for (int i = 0; i < sol->trucks; ++i) {
    Route* r = sol->routes[i];
    if (r->len == EMPTY) {
      free_route(sol->routes[i]);
      sol->routes[i] = (Route *) NULL;
      continue;
    }
    r->tail->prev->next = sol->unrouted;  // only prepend non-depot nodes
    if (sol->unrouted) {
      sol->unrouted->prev = r->tail->prev;
    }
    sol->unrouted = r->nodes->next;  // set unrouted to the new head
    sol->unrouted->prev = (Node*) NULL;
    r->nodes->next = r->tail;  // there are only two depots left
    r->tail->prev = r->nodes;
    free_route(sol->routes[i]);
    sol->routes[i] = (Route*) NULL;
  }
  sol->num_unrouted = num_nodes - 1;  // exclude the depot
  sol->trucks = 0;
  sol->workers_cache = 0;
  sol->dist_cache = 0;
  sol->time = 0;
  sol->saturation_time = 0;
}


//! Save the details of a solution to a file.
void save_solution_details(Solution* sol, Config* cfg) {
  FILE* outfile = fopen(cfg->sol_details_filename, "a");
  fprint_solution(outfile, sol, cfg, BASIC_VERBOSITY);
  fprintf(outfile, "\n");
  fclose(outfile);
}
