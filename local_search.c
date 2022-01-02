/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "config.h"
#include "node.h"
#include "problemreader.h"
#include "route.h"
#include "solution.h"
#include "tabu_search.h"
#include "local_search.h"

///////////////////////////////////////////////////////////////////////////////
// File Scope (Static) Functions                                             //
///////////////////////////////////////////////////////////////////////////////
static inline double calc_delta_dist_move(double** d, Node* first, Node* last,
                                          Node* after);
static int delta_is_higher(Move* m, int d_trucks, int d_workers, double d_dist);
static int empty_route(Solution*, int route_idx);
static int move_reduces_workers(Route* source, Node* first, Node* last,
                                int min_reduction);
static int swap_node(Route* r1, Route* r2);


//! Return the distance difference when moving a sequence of nodes.
//! \param d The distance matrix.
//! \return A positive delta if the distance can be reduced by the move.
static inline double calc_delta_dist_move(double** d, Node* first, Node* last,
                                          Node* after) {
  return d[first->prev->id][first->id] + d[last->id][last->next->id] -
         d[first->prev->id][last->next->id] +
         d[after->id][after->next->id] -
         d[after->id][first->id] - d[last->id][after->next->id];
}


//! Return true if the passed delta values are higher than the move's.
//! Doing so, follow the hierarchical objective function stating that
//! trucks are better than workers and workers are better than distance.
int delta_is_higher(Move* m, int d_trucks, int d_workers, double d_dist) {
  if (d_trucks && ! m->delta_trucks)
    return 1;
  else if (d_trucks == m->delta_trucks) {
    if (d_workers > m->delta_workers)
      return 1;
    else if ((d_workers == m->delta_workers) &&
      (d_dist - MIN_DELTA > m->delta_dist))
      return 1;
  }
  return 0;
}


//! Attempt to remove all nodes from a route and return true on success.
//! This function potentially worsens a solution, so it is usually best to use
//! it on cloned solutions.
//! The function stops as soon as one node cannot be moved to another route.
static int empty_route(Solution* sol, int route_idx) {
  Route *source = sol->routes[route_idx];
  Node *n = source->nodes->next;
  Insertion ins = {NULL, NULL, NULL, INFINITY, 0.0, NULL, NULL};
  if (source->len == EMPTY)
    return 1;
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
      break;
    remove_nodes(source, ins.node, ins.node);
    add_nodes(ins.target, ins.node, ins.node, ins.after);
    if (source->len == EMPTY)
      return 1;
  }
  return 0;
}


//! Return the number of workers that can be removed by removing first to last.
//! \param first The first node in a list of nodes considered to be moved away.
//! \param last The last node in a list of nodes considered to be moved away.
//! \param min_reduction Only investigate reductions >= min_reduction.
//! \return The number of workers that can be reduced.
static int move_reduces_workers(Route* source, Node* first, Node* last,
                                int min_reduction) {
  int max_reduction = source->workers - 1;  // one worker (driver) is needed
  if (!min_reduction) min_reduction++;
  int reduction = 0;
  first->prev->next = last->next;
  last->next->prev = first->prev;
  while ((min_reduction <= max_reduction) &&
         is_feasible_with(source, source->workers - min_reduction))
    reduction = min_reduction++;
  first->prev->next = first;
  last->next->prev = last;
  return reduction;
}


//! Perform the first feasible and useful swap operation between r1 and r2.
//! A swap is useful if it decreases the total distance.
//! \return 1 if the distance was reduced, otherwise 0
static int swap_node(Route* r1, Route* r2) {
  double savings = 0.0;
  double capacity = r1->pb->capacity;
  double** d = r1->pb->c_m[0];  // distance
  double** c_m1 = r1->pb->c_m[r1->workers];  // driving + service time
  double** c_m2 = r2->pb->c_m[r2->workers];  // driving + service time
  Node* n1 = r1->nodes->next;  // start w/ the first node after the depot
  Node* n2 = r2->nodes->next;
  while (n1->next) {
    while (n2->next) {
      // check capacity
      if (capacity < r1->load - n1->demand + n2->demand ||
        capacity < r2->load - n2->demand + n1->demand) {
        n2 = n2->next;
        continue;
      }
      // check time windows
      n1->aest_cache = max(n2->prev->aest + c_m2[n2->prev->id][n1->id],
                            n1->est);  //  when do we get to n1 on r2
      n2->aest_cache = max(n1->prev->aest + c_m1[n1->prev->id][n2->id],
                            n2->est);  //  when do we get to n2 on r1
      if (n1->aest_cache <= n1->lst && n2->aest_cache <= n2->lst) {
        n1->next->aest_cache = max(n2->aest_cache +
                                    c_m1[n2->id][n1->next->id],
                                    n1->next->est);
        n2->next->aest_cache = max(n1->aest_cache +
                                    c_m2[n1->id][n2->next->id],
                                    n2->next->est);
        if (n1->next->aest_cache <= n1->next->alst &&
            n2->next->aest_cache <= n2->next->alst) {
          // check savings
          savings = d[n1->prev->id][n1->id] + d[n1->id][n1->next->id] +
                    d[n2->prev->id][n2->id] + d[n2->id][n2->next->id] -
                    d[n1->prev->id][n2->id] - d[n2->id][n1->next->id] -
                    d[n2->prev->id][n1->id] - d[n1->id][n2->next->id];
          if (savings > MIN_DELTA) {
            swap(r1, r2, n1, n2);
            return 1;
          }
        }
      }
      n2 = n2->next;
    }
    n2 = r2->nodes->next;
    n1 = n1->next;
  }
  return 0;
}


///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////

//! Try to reduce trucks by attempting to move all of a truck's nodes.
//! As the moves are generally increasing the distance, this would reduce the
//! solution quality. Hence, the "result" is only committed if all the nodes
//! can be removed and not just some.
int brute_reduce_trucks(Solution** sol_ptr) {
  Solution* sol = *sol_ptr;
  Solution* clone = clone_solution(sol);
  int reduced = 0, improved = 0;
  do {
    for (int i = 0; i < clone->trucks; ++i) {
      reduced = empty_route(clone, i);
      if (reduced) {
        remove_route(clone, i);
        free_solution(sol);
        sol = clone_solution(clone);
        *sol_ptr = sol;  // set sol_ptr to the new solution's address
        improved = 1;
        break;
      }
    }
  } while (reduced);
  free_solution(clone);
  return improved;
}


//! Perform a full local search.
//! First reduce trucks and distance, then workers and distance, then distance.
Solution* do_ls(Solution *sol) {
  if (sol->pb->cfg->do_ls) {
    sol = reduce_trucks(sol);
    if (sol->pb->cfg->max_workers > 1)
      reduce_workers(sol);
    // reduce_distance(sol);
  } else  // if local search is disabled, at least unused workers are removed
    for (int i = 0; i < sol->trucks; ++i) {
      reduce_service_workers(sol->routes[i]);
    }
  return sol;
}


//! Perform all feasible and useful move operations.
//! A move is useful if it decreases the number of trucks or workers or the
//! total distance.
//! \return 1 if at least one move was performed, otherwise 0.
int move_all(Solution* sol, int state) {
  if (sol->pb->cfg->best_moves)
    return move_all_best(sol, state);
  int len = (int) sol->pb->cfg->max_move;
  int updated = 0, success = 0, delta_trucks = 0;
  Move m; init_move(&m, IMPROVING);
  while (len) {
    do {
      updated = 0;
      for (int i = sol->trucks - 1; i >= 1; --i) {
        for (int j = i - 1; j >= 0; --j) {
          updated |= update_move(&m, sol->routes[j], sol->routes[i], state,
                                 len);
          delta_trucks = m.delta_trucks;
          perform_move(sol, &m);
          if (delta_trucks) break;  // route j wouldn't exist anymore
          updated |= update_move(&m, sol->routes[i], sol->routes[j], state,
                                 len);
          delta_trucks = m.delta_trucks;
          perform_move(sol, &m);
          if (delta_trucks) break;  // route i wouldn't exist anymore
        }
      }
      success |= updated;
    } while (updated);
    len--;
  }
  return success;
}


//! Perform all best move operations.
//! A move is considered best if it is feasible and decreases the cost for
//! trucks, workers and the total distance more than any other feasible move.
//! Only inter-route moves are considered.
int move_all_best(Solution* sol, int state) {
  int updated = 0, success = 0;
  Move m; init_move(&m, IMPROVING);
  do {
    updated = 0;
    for (int i = sol->trucks - 1; i >= 1; --i) {
      for (int j = i - 1; j >= 0; --j) {
        updated |= update_move(&m, sol->routes[j], sol->routes[i], state, 2);
        updated |= update_move(&m, sol->routes[i], sol->routes[j], state, 2);
        updated |= update_move(&m, sol->routes[j], sol->routes[i], state, 1);
        updated |= update_move(&m, sol->routes[i], sol->routes[j], state, 1);
      }
    }
    perform_move(sol, &m);
    success |= updated;
  } while (updated);
  return success;
}


//! Perform the given move and reset it afterwards.
void perform_move(Solution* sol, Move* m) {
  if (!m->first)
    return;
  update_tabulist_move(sol->pb->tl, m);
  if (m->delta_trucks) {
    remove_nodes_noupdate(m->source, m->first, m->last);
    remove_route(sol, get_route_index(sol, m->source->id));
  } else if (m->delta_workers) {
    remove_nodes_noupdate(m->source, m->first, m->last);
    m->source->workers -= m->delta_workers;
    calc_ests(m->source, m->source->nodes, m->source->workers);
    calc_lsts(m->source, m->source->tail, m->source->workers);
  } else {
    remove_nodes(m->source, m->first, m->last);
  }
  add_nodes(m->target, m->first, m->last, m->after);
  init_move(m, m->improving);
}


// TODO: implement
// void reduce_distance(Solution *sol)
// {
// }


//! Try to reduce the number of trucks used by the solution.
//! Using brute_reduce_trucks at the beginning of the loop
//! performs slightly better than at the end of the loop
//! (however, at the cost of occasionally increasing the
//! number of service workers and the total distance).
Solution *reduce_trucks(Solution *sol) {
  int improved = 0;
  do {
    improved = 0;
    improved |= brute_reduce_trucks(&sol);
    improved |= move_all(sol, REDUCE_TRUCKS);
    improved |= swap_all(sol);
  } while (improved);
  return sol;
}


//! Try to reduce the number of workers used by the solution.
//! First, superfluous workers are removed before a local search tries to
//! improve the solution further.
void reduce_workers(Solution *sol) {
  int improved = 0;
  for (int i = 0; i < sol->trucks; ++i) {
    reduce_service_workers(sol->routes[i]);
  }
  do {
    improved = 0;
    improved |= move_all(sol, REDUCE_WORKERS);
    improved |= swap_all(sol);
  } while (improved);
}


//! Perform all feasible and useful swap operations.
//! A swap is useful if it decreases the number of workers or the
//! total distance.
int swap_all(Solution* sol) {
  long int len = sol->pb->cfg->max_swap;
  int improved = 0;
  int success = 0;
  do {
    improved = 0;
    if (len >= 1) {
      for (int i = sol->trucks - 1; i >= 1; --i) {
        for (int j = i - 1; j >= 0; --j) {
          improved |= swap_node(sol->routes[i], sol->routes[j]);
        }
      }
    }
    success |= improved;
  } while (improved);
  return success;
}


//! Update the given move if there is a better move from source to target.
//! Better means that the move is allowed (feasible and not tabu) and its
//! savings are higher.
//! NOTE: Checking the insertion's feasibility after checking for an actual
//! improvement performs better on average over R101-R112 for best moves.
//! \param len The number of nodes to be moved at once. Eg. len = 2 => move2.
int update_move(Move* m, Route* source, Route* target, int state, int len) {
  if (source->pb->cfg->max_move < len) return 0;
  if (source->len < (EMPTY + len)) return 0;
  int updated = 0;
  int delta_trucks = ((source->len) == (EMPTY + len));
  int delta_workers = delta_trucks ? (source->workers) : 0;
  double delta_dist = 0.0;
  if ((m->delta_trucks == 1) && !delta_trucks)
    return 0;  // truck can't be reduced but is reduced in the best move
  Node* after = target->nodes;
  Node* first = source->nodes->next;
  Node* last = first;
  while (--len)
    last = last->next;
  while (last->next) {  // n is not the closing depot
    if (target->pb->capacity < target->load + sum_demands(first, last)) {
      first = first->next;
      last = last->next;
      continue;
    }
    if ((state >= REDUCE_WORKERS) && !delta_trucks)
      delta_workers = move_reduces_workers(source, first, last,
                                           m->delta_workers);
    while (after != target->tail) {
      delta_dist = calc_delta_dist_move(source->pb->c_m[0], first, last,
                                        after);
      if (delta_is_higher(m, delta_trucks, delta_workers, delta_dist)) {
        if (can_insert(target, first, last, after)) {
          Move candidate = {.source = source, .target = target, .first = first,
            .last = last, .after = after, .delta_dist = delta_dist,
            .delta_trucks = delta_trucks, .delta_workers = delta_workers,
            .improving = m->improving};
          if (!is_move_tabu(source->pb->tl, &candidate)) {
            *m = candidate;
            if (!source->pb->cfg->best_moves)
              return 1;
            updated = 1;
          }
        }
      }
      after = after->next;
    }
    after = target->nodes;
    first = first->next;
    last = last->next;
  }
  return updated;
}

