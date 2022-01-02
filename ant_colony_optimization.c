/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <limits.h>
#include <math.h>
#include <stdio.h>
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
#include "ant_colony_optimization.h"


///////////////////////////////////////////////////////////////////////////////
// File Scope (Static) Functions                                             //
///////////////////////////////////////////////////////////////////////////////

static int calc_aco_insertion(Route *, Node *, Insertion *);
static int calc_mr_insertion(Route *, Node *, Insertion *);
static Insertion *calc_next_insertion(Route *, Node *n, Node *after);
static double calc_trail(double** p_m, int depot_id, int pred_id, int succ_id,
                         int node_id);
static Node* get_parallel_seed(Solution*);
static Insertion* init_parallel_insertions(Solution*);
static void init_parallel_routes(Solution*, int workers);
static Insertion* prepend_insertions(Insertion *, Route *, Node *)
  __attribute__ ((warn_unused_result));
static void solve_parallel_aco(Solution* sol, int workers);
static void solve_solomon_aco(Solution*, int workers);
static void solve_solomon_mr(Solution*, int workers);
static Insertion* update_insertions(Insertion* old, Insertion* ins,
                                    Node* unrouted)
  __attribute__ ((warn_unused_result));


//! Pick one of the given insertions using a weighted roulette wheel mechanism.
Insertion* aco_pick_insertion(Insertion insertions[],
                              int num_insertions, double min_cost) {
  double cum_attractiveness = 0.0;
  Insertion* ins = (Insertion*) NULL;
  double threshold = 0.0;

  min_cost -= 1.0; // allows normalizing cost to [1, inf)
  for (int i = 0; i < num_insertions; ++i) {
    insertions[i].attractiveness = 1 / (insertions[i].cost -
                                        min_cost);
    cum_attractiveness += insertions[i].attractiveness;
  }
  threshold = drand48() * cum_attractiveness;
  for (int i = 0; i < num_insertions; ++i) {
    cum_attractiveness -= insertions[i].attractiveness;
    if (threshold >= cum_attractiveness) {
      ins = &insertions[i];
      break;
    }
  }
  return ins;
}


//! Calculate the cheapest insertions position for node on route.
//! The position is returned using the passed ins pointer.
//! \param ins The insertion data to be updated.
//! \param node The node to be inserted.
//! \param route The route on which the given node is to be inserted.
//! \return 1 if at least one position is possible, 0 otherwise.
static int calc_aco_insertion(Route *route, Node *node, Insertion *ins) {
  Node* after = route->nodes;
  double cost_dist = 0.0, cost_time = 0.0, cost = 0.0;
  double est_node = 0.0, est_succ = 0.0;
  double** d = route->pb->c_m[0]; // distance
  double** c_m = route->pb->c_m[route->workers];
  double alpha = route->pb->cfg->alpha, alpha2 = 1.0 - alpha;
  double mu = route->pb->cfg->mu;
  double lambda = route->pb->cfg->lambda;
  double** p_m = route->pb->pheromone;
  double trail = 1.0;
  int updated = 0;

  if (route->pb->capacity < route->load + node->demand)
    return 0;
  while (after != route->tail) {
    if (!can_insert_one(route, node, after)) {
      after = after->next;
      continue;
    }
    cost_dist = d[after->id][node->id] + d[node->id][after->next->id] -
    mu * d[after->id][after->next->id];
    if (alpha2) {
      est_node = max(node->est, after->aest + c_m[after->id][node->id]);
      est_succ = max(after->next->aest, est_node +
                     c_m[node->id][after->next->id]);
      cost_time = est_succ - after->next->aest;
    }
    cost = alpha * cost_dist + alpha2 * cost_time;
    cost = cost - lambda * d[DEPOT][node->id];
    trail = calc_trail(p_m, route->depot_id, after->id, after->next->id,
                       node->id);
    cost = (cost >= 0) ? (cost / trail) : (cost * trail);
    if (cost < ins->cost) {
      ins->target = route;
      ins->node = node;
      ins->after = after;
      ins->cost = cost;
      updated = 1;
    }
    after = after->next;
  }
  return updated;
}


// TODO: document and compare to _aco_ version
static int calc_mr_insertion(Route* route, Node* node, Insertion* ins) {
  Node* after = route->nodes;
  double cost_dist = 0.0, cost_time = 0.0, cost = 0.0, attract = 0.0;
  double est_node = 0.0, est_succ = 0.0;
  double** d = route->pb->c_m[0];  // distance matrix
  double** c_m = route->pb->c_m[route->workers];
  double alpha = route->pb->cfg->alpha, alpha2 = 1.0 - alpha;
  double mu = route->pb->cfg->mu;
  double lambda = route->pb->cfg->lambda;
  double** p_m = route->pb->pheromone;
  double trail = 1.0;
  int updated = 0;

  if (route->pb->capacity < route->load + node->demand)
    return 0;
  while (after != route->tail) {
    if (!can_insert_one(route, node, after)) {
      after = after->next;
      continue;
    }
    cost_dist = d[after->id][node->id] + d[node->id][after->next->id] -
    mu * d[after->id][after->next->id];
    if (alpha2) {
      est_node = max(node->est, after->aest + c_m[after->id][node->id]);
      est_succ = max(after->next->aest, est_node +
      c_m[node->id][after->next->id]);
      cost_time = est_succ - after->next->aest;
    }
    cost = alpha * cost_dist + alpha2 * cost_time;
    attract = lambda * d[DEPOT][node->id] - cost;
    trail = calc_trail(p_m, route->depot_id, after->id, after->next->id,
                       node->id);
    if (attract < 0.0)
      attract = MIN_DELTA;
    attract *= trail;
    if (attract > ins->attractiveness) {
      ins->target = route;
      ins->node = node;
      ins->after = after;
      ins->attractiveness = attract;
      updated = 1;
    }
    after = after->next;
  }
  return updated;
}


//! Return the first possible insertion position of n behind after.
//! If there is no feasible position, return NULL.
//! \param route The target route (n is attempted to be inserted into it).
static Insertion *calc_next_insertion(Route *route, Node *n, Node *after) {
  double **d = route->pb->c_m[0]; // distance matrix
  double **c_m = route->pb->c_m[route->workers];
  double cost_dist = 0.0, cost_time = 0.0, cost = 0.0;
  double est_node = 0.0, est_succ = 0.0;
  double trail = 1.0;
  double **p_m = route->pb->pheromone;
  Insertion *ins = (Insertion *) NULL;
  double alpha = route->pb->cfg->alpha, alpha2 = 1 - alpha;
  if (route->pb->capacity < route->load + n->demand)
    return (Insertion *) NULL;
  while (!can_insert_one(route, n, after)) {
    if (after->next == route->tail)
      return (Insertion *) NULL;
    after = after->next;
  }
  ins = (Insertion *) s_malloc(sizeof(Insertion));
  // own (non-solomon) attractiveness calculation
  // reasoning: distance from the depot doesn't play a role for the insertion
  // (saving) cost b/c there will not be a truck for just this customer
  cost_dist = d[after->id][n->id] + d[n->id][after->next->id] -
  route->pb->cfg->mu * d[after->id][after->next->id];
  if (alpha2) {
    est_node = max(n->est, after->aest + c_m[after->id][n->id]);
    est_succ = max(after->next->aest, est_node + c_m[n->id][after->next->id]);
    cost_time = est_succ - after->next->aest;
  }
  cost = alpha * cost_dist + alpha2 * cost_time;
  trail = calc_trail(p_m, route->depot_id, after->id, after->next->id, n->id);
  if (cost > MIN_COST)
    ins->attractiveness = trail / cost;
  else
    ins->attractiveness = trail / MIN_COST;
  ins->cost = -1.0;
  ins->node = n;
  ins->after = after;
  ins->target = route;
  ins->prev = (Insertion *) NULL;
  ins->next = (Insertion *) NULL;
  return ins;
}


//! Return the trail of inserting node between after and after->next.
//! \param depot_id The receiving route's depot id.
//! The pheromone would not work if all depot's had the same id. Hence,
//! virtual depots are added to the pheromone after the last regular
//! node - one for each route. A route's id serves as its depot id.
static inline double calc_trail(double** p_m, int depot_id, int after_id,
                                int succ_id, int node_id) {
  if (after_id == DEPOT)
    after_id = depot_id;
  if (succ_id == DEPOT)
    succ_id = depot_id;
  return (p_m[after_id][node_id] + p_m[node_id][succ_id]) /
         (2.0 * p_m[after_id][succ_id]);
}


//! Return one of the most promising seed nodes for parallel construction.
//! The quality of the seed is solely determined by a roulette wheel selection
//! of a candidate's pheromone neighbourhood to the starting depot. The
//! underlying logic is that two nodes that were next to the
//! starting depot were automatically on different routes.
//! Return NULL if there are no candidates available.
static Node* get_parallel_seed(Solution* sol) {
  Node *nl = sol->unrouted;
  double **p_m = sol->pb->pheromone;
  double cum_attractiveness = 0.0;
  double threshold = 0.0;
  int num_nodes = sol->pb->num_nodes;
  double trail[sol->num_unrouted];
  double* trail_ptr = trail;
  while (nl) {
    (*trail_ptr) = (p_m[num_nodes + sol->trucks][nl->id] +
                    p_m[nl->id][num_nodes + sol->trucks]);
    cum_attractiveness += (*trail_ptr);
    trail_ptr++;
    nl = nl->next;
  }
  nl = sol->unrouted;
  trail_ptr = trail;
  threshold = drand48() * cum_attractiveness;
  while (nl) {
    cum_attractiveness -= (*trail_ptr);
    if (threshold >= cum_attractiveness) return nl;
    trail_ptr++;
    nl = nl->next;
  }
  fprintf(stderr, "ERROR: get_parallel_seed: no seed selected\n");
  return (Node *) NULL;
}


//! Return all feasible insertions for all nodes.
//! These include all feasible positions of all unrouted nodes to each route.
//! Each of the insertions is separately malloced.
static Insertion *init_parallel_insertions(Solution *sol) {
  Node *unrouted = sol->unrouted;
  Insertion *insertions = (Insertion *) NULL;
  while (unrouted) {
    for (int i = 0; i < sol->trucks; ++i) {
      insertions = prepend_insertions(insertions, sol->routes[i], unrouted);
    }
    unrouted = unrouted->next;
  }
  return insertions;
}


//! Initialize a number of parallel routes.
//! The number of routes is
//! the best lowest known number of trucks so far which is reduced by one
//! until the algorithm beliefs a further reduction is not possible.
static void init_parallel_routes(Solution *sol, int workers) {
  Node *unrouted = (Node *) NULL;
  Problem *pb = sol->pb;
  int max_trucks = pb->sol->trucks;  // best (min) known number of trucks
  if (!max_trucks) {  // there was no past solution
    solve_solomon(pb->sol, workers, pb->num_nodes); // initialize truck number
    max_trucks = pb->sol->trucks;
  }
  if (pb->state == REDUCE_TRUCKS)
    max_trucks--;
  for (int i = 0; i < max_trucks; ++i) {
    unrouted = get_parallel_seed(sol);
    remove_unrouted(sol, unrouted);
    new_route(sol, unrouted, workers);
  }
}


//! Prepending all possible insertions of n to r.
//! \return new head of the insertion list
// research result: only adding the best position like in I1 worsens the
// overall solution quality even more in comparison to I1
static Insertion* prepend_insertions(Insertion *ins, Route *r, Node *n) {
  Insertion *head = (Insertion *) NULL;
  Node *after = r->nodes;
  while (after != r->tail) {
    head = calc_next_insertion(r, n, after);
    if (!head) break;
    if (!ins) {
      ins = head;
      after = head->after->next;
      continue;
    }
    ins->prev = head;
    head->next = ins;
    ins = head;
    after = head->after->next;
  }
  return ins;
}


//! Construct a solution's routes in parallel.
//! Given an initial truck number in pb->sol->trucks all routes are constructed
//! in parallel thus increasing the degree of freedom.
//! The max number of trucks is one less than the best known until none of the
//! ants in an entire run has found a feasible solution.
// Research result: doing ls to move to feasible solution instead of adding
// more trucks reduces overall solution quality.
static void solve_parallel_aco(Solution *sol, int workers) {
  Insertion *ins = (Insertion *) NULL;
  init_parallel_routes(sol, workers);
  Insertion *insertions = init_parallel_insertions(sol);
  while (insertions) {
    // hack :(
    ins = pick_insertion(&(Insertion_List) {.head = insertions,
                                            .tail = (Insertion*) NULL,
                                            .size = 0,
                                            .max_size = LONG_MAX},
                         USE_WEIGHTS);
    remove_unrouted(sol, ins->node);
    add_nodes(ins->target, ins->node, ins->node, ins->after);
    insertions = update_insertions(insertions, ins, sol->unrouted);
  }
  // TODO: deal with remaining unrouted nodes (shake to move to
  // feasible solution) (meanwhile simply add them via solomon)
  if (!sol->unrouted) {
    sol->pb->attempts = 0;
  } else {
    sol->pb->attempts++;
    if (sol->pb->attempts >= sol->pb->cfg->max_failed_attempts
      && sol->pb->state == REDUCE_TRUCKS) {
      sol->pb->state++;
    sol->pb->attempts = 0;
      }
  }
  solve_solomon_aco(sol, workers);
}


//! Create an initial solution using Solomon's I1 heuristic.
//! The heuristic has been adapted for the ACO metaheuristic.
static void solve_solomon_aco(Solution *sol, int workers) {
  Node *unrouted = NULL;
  Route *route = NULL;
  Insertion ins = {route, NULL, NULL, INFINITY, 0.0, NULL, NULL};
  Insertion insertions[sol->num_unrouted];
  double min_cost = INFINITY;

  for (int i = 0; i < sol->num_unrouted; ++i) {
    insertions[i].cost = INFINITY;
    insertions[i].attractiveness = 0.0;
    insertions[i].target = (Route *) NULL;
    insertions[i].node = (Node *) NULL;
    insertions[i].after = (Node *) NULL;
    insertions[i].prev = (Insertion *) NULL;
    insertions[i].next = (Insertion *) NULL;
  }
  while (sol->unrouted) {
    unrouted = get_seed(sol);
    remove_unrouted(sol, unrouted);
    route = new_route(sol, unrouted, workers);
    while (sol->unrouted) {  // fill the current route
      min_cost = INFINITY;
      unrouted = sol->unrouted;
      for (int i = 0; i < sol->num_unrouted; ++i) {
        insertions[i].cost = INFINITY; // reset rel. part of ins
        insertions[i].node = (Node *) NULL;
        calc_aco_insertion(route, unrouted, &insertions[i]);
        min_cost = (insertions[i].cost < min_cost) ?
          insertions[i].cost : min_cost;
        unrouted = unrouted->next;
      }
      if (isinf(min_cost)) break;
      ins = *aco_pick_insertion(insertions, sol->num_unrouted, min_cost);
      remove_unrouted(sol, ins.node);
      add_nodes(ins.target, ins.node, ins.node, ins.after);
      ins.node = (Node *) NULL;
    }
  }
}


//! Create an initial solution using Solomon's I1 heuristic.
//! The difference to solve_solomon_aco is that the attractiveness is
//! calculated directly and that negative attractivenesses are simply replaced
//! by MIN_DELTA.
//! Compared to solve_solomon_aco implementation no significant difference
//! could be found in the solution quality or processing time.
static void solve_solomon_mr(Solution *sol, int workers) {
  Node* unrouted = NULL;
  Route* route = NULL;
  Insertion ins = {route, NULL, NULL, INFINITY, -INFINITY, NULL, NULL};
  Insertion insertions[sol->num_unrouted];
  double max_attr = -INFINITY;
  for (int i = 0; i < sol->num_unrouted; ++i) {
    insertions[i].cost = INFINITY;
    insertions[i].attractiveness = -INFINITY;
    insertions[i].target = (Route *) NULL;
    insertions[i].node = (Node *) NULL;
    insertions[i].after = (Node *) NULL;
    insertions[i].prev = (Insertion *) NULL;
    insertions[i].next = (Insertion *) NULL;
  }
  while (sol->unrouted) {
    unrouted = get_seed(sol);
    remove_unrouted(sol, unrouted);
    route = new_route(sol, unrouted, workers);
    while (sol->unrouted) {  // fill the current route
      unrouted = sol->unrouted;
      max_attr = -INFINITY;
      for (int i = 0; i < sol->num_unrouted; ++i) {
        insertions[i].attractiveness = -INFINITY;  // reset rel. part of ins
        insertions[i].node = (Node *) NULL;
        calc_mr_insertion(route, unrouted, &insertions[i]);
        max_attr = (insertions[i].attractiveness > max_attr) ?
          insertions[i].attractiveness : max_attr;
        unrouted = unrouted->next;
      }
      if (isinf(max_attr)) break;
      ins = *pick_insertion_from_array(insertions, sol->num_unrouted);
      remove_unrouted(sol, ins.node);
      add_nodes(ins.target, ins.node, ins.node, ins.after);
      ins.node = (Node*) NULL;
    }
  }
}


//! Update the given insertion list by removing all invalid insertions and
//! adding potential new insertions.
//! \ins The insertion that was performed.
static Insertion *update_insertions(Insertion *old, Insertion *ins,
                                    Node *unrouted) {
  Route *r = ins->target;
  ins = remove_invalid_insertions(old, ins); // frees old ins
  while (unrouted) {
    ins = prepend_insertions(ins, r, unrouted);
    unrouted = unrouted->next;
  }
  return ins;
}


///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////


//! Select and run a route construction heuristic for ACO.
void aco_construct_routes(Solution* sol, int workers) {
  switch (sol->pb->cfg->start_heuristic) {
    case SOLOMON:
      solve_solomon_aco(sol, workers);
      return;
    case PARALLEL:
      solve_parallel_aco(sol, workers);
      return;
    case SOLOMON_MR:
      solve_solomon_mr(sol, workers);
      return;
  }
  fprintf(stderr, "ERROR: start heuristic %s not available for ACO.\n",
          START_HEURISTICS[sol->pb->cfg->start_heuristic]);
  fprintf(stderr, "pick any of\n%s\n%s\n%s\n",
          START_HEURISTICS[SOLOMON],
          START_HEURISTICS[SOLOMON_MR],
          START_HEURISTICS[PARALLEL]);
}


//! Solve the given problem using the ACO metaheuristic.
//! The way the pheromone between the depot nodes and the regular nodes is
//! handled is critical to the success (convergence) of the ACO.
//! A common working solution is to have different virtual depot nodes for each
//! of the routes in the solution. In our case this is done by a virtual depot
//! id.
void solve_aco(Problem* pb, int workers) {
  double best_cost = INFINITY;
  double cost = INFINITY;
  Solution* sol = new_solution(pb);
  Solution* temp = NULL;
  while (proceed(pb, (unsigned long) pb->num_solutions)) {
    for (int i = 0; i < pb->cfg->ants; ++i) {  // solve once for each ant
      reset_solution(sol, pb->num_nodes);
      aco_construct_routes(sol, workers);

      // TODO: calculate a hash, look if it's stored in a binary
      // search or AA tree or hashtable or the like
      // if so, continue with next ant, else add elem to search tree and
      // continue w/ local search

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
    pb->num_solutions += pb->cfg->ants;
    update_pheromone(pb, pb->sol);
  }
  free_solution(sol);
}


// TODO: implement adaptive fast aco; if successful give proper name
//! Solve the given problem using the ACO metaheuristic.
//! The way the pheromone between the depot nodes and the regular nodes is
//! handled is critical to the success (convergence) of the ACO.
//! A common working solution is to have different virtual depot nodes for each
//! of the routes in the solution. In our case this is done by a virtual depot
//! id.
void solve_gaco(Problem* pb, int workers) {
  double best_cost = INFINITY, local_best_cost = INFINITY;
  double cost = INFINITY;
  int count = 0;
  Solution* sol = new_solution(pb);
  Solution* temp = NULL;
  while (proceed(pb, (unsigned long) pb->num_solutions)) {
    for (int i = 0; i < pb->cfg->ants; ++i) {  // solve once for each ant
      aco_construct_routes(sol, workers);

      // TODO: if the aco is stuck, add long term memory to avoid same
      // solution areas
      if (drand48() >= 0.0) {
        sol = do_ls(sol);
      } else {
        for (int i = 0; i < sol->trucks; ++i) {
          reduce_service_workers(sol->routes[i]);
        }
      }

      cost = calc_costs(sol, pb->cfg);

//       if (cost < local_best_cost) {
//         local_best_cost = cost;
//         print_progress(sol);  // TODO: maybe remove
//       } else
      if (fabs(local_best_cost - cost) < 0.001 && count >= 2) {  // maybe
            // restrict to min # of identical local_best_costs
        count = 0;
        fprintf(stderr, "resetting pheromone\n");
//         sol = do_ls(sol);
//         cost = calc_costs(sol, pb->cfg);
//         if (cost < local_best_cost) {
        local_best_cost = cost;
        print_progress(sol);  // TODO: maybe remove
//         } else {
        set_double_matrix(pb->pheromone, (size_t) (2 * pb->num_nodes - 1),
                          (size_t) (2 * pb->num_nodes - 1),
                          pb->cfg->initial_pheromone);
        local_best_cost = INFINITY;
//         }
      } else if (fabs(local_best_cost - cost) < 0.001) {
        count++;
      }

      if (cost < best_cost) {
        count = 0;
        best_cost = cost;
        sol->time = time((time_t *)NULL) - pb->start_time;
        print_progress(sol);
        temp = pb->sol;
        pb->sol = sol;
        sol = temp;
      }
      reset_solution(sol, pb->num_nodes);
    }
    pb->num_solutions += pb->cfg->ants;
    update_pheromone(pb, pb->sol);
  }
  free_solution(sol);
}


//! Update the pheromone information.
//! The persistance is determined by a constant factor \rho and the
//! reinforcement is (1 - persistance) for all nodes i having j as a
//! successor in the given solution, otherwise 0.
//! The size of the pheromone matrix is (2n+1)x(2n+1) where n is the number of
//! customers not including the depot. Technically, (2n)x(2n) would suffice,
//! but the first row and column are ignored to avoid indexing errors (this
//! way, the node's ids can be used to index the matrix directly.
//! The 0 depot is ignored because it is equal for all routes. Instead, each
//! route gets a separate "virtual depot" id being the number of nodes + the
//! index of the route (hence starting with 101 if there are 100 customers).
//! The resulting matrix looks like
//!
//! i.........i
//! .n...nc...c
//! ..   ..   .
//! ..   ..   .
//! ..   ..   .
//! .n   nc...c
//! .s...si...i
//! ..   ..   .
//! ..   ..   .
//! ..   ..   .
//! is...si...i
//!
//! where
//! 'i' denotes irrelevant elements
//! 'n' customer with id row is followed by customer with id col in best known
//!     solution
//! 's' row denotes id of virtual starting depot, col denotes first customer
//!     on a route (after its virtual starting depot)
//! 'c' row denotes last customer on a route (before its virtual closing
//!     depot), col denotes id of virtual closing depot
void update_pheromone(Problem *pb, Solution *sol) {
  Node* n = (Node*) NULL;
  double** p_m = pb->pheromone;
  double rho = pb->cfg->rho, min_pheromone = pb->cfg->min_pheromone;
  double new_pheromone = 1.0 - rho;
  int num_nodes = pb->num_nodes;
  for (int i = 1; i < (2 * num_nodes - 1); ++i) {  // ignore 0 DEPOT
    for (int j = 1; j < (2 * num_nodes - 1); ++j) {
      p_m[i][j] = max(p_m[i][j] * rho, min_pheromone);
    }
  }
  for (int r = 0; r < sol->trucks; ++r) {
    p_m[num_nodes + r][sol->routes[r]->nodes->next->id] += new_pheromone;
    p_m[sol->routes[r]->tail->prev->id][num_nodes + r] += new_pheromone;
    n = sol->routes[r]->nodes->next->next;  // ignore the starting depot node
    while (n->next) {  // ignore the ending depot node
      p_m[n->prev->id][n->id] += new_pheromone;
      n = n->next;
    }
  }
  #ifdef DEBUG
  if (pb->cfg->verbosity >= FULL_DEBUG) {
    printf("\n");
    fprint_solution(stdout, sol, pb->cfg, 1);
    print_double_matrix(2 * num_nodes - 1, pb->pheromone, "pheromone");
  }
  #endif
}

