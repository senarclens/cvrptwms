/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "node.h"
#include "problemreader.h"
#include "solution.h"
#include "vrptwms.h"
#include "wrappers.h"
#include "route.h"

///////////////////////////////////////////////////////////////////////////////
// File Scope (Static) Functions                                             //
///////////////////////////////////////////////////////////////////////////////
static void free_insertions(Insertion* insertions);

//! Free the memory of the given insertions.
static void free_insertions(Insertion* insertions) {
  while (insertions) {
    Insertion* temp = insertions->next;
    free(insertions);
    insertions = temp;
  }
}


///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////

//! "Constructor".
//! Once constructed, the route is added to the solution and the number of
//! trucks incremented.
Route* new_route(Solution* sol, Node* seed, int workers) {
  Route* route = (Route*) s_malloc(sizeof(Route));
  route->pb = sol->pb;
  route->depot_id = route->pb->num_nodes + sol->trucks;
  route->id = sol->trucks;
  sol->routes[sol->trucks] = route;
  sol->trucks++;
  route->nodes = new_depot(route->pb);
  route->nodes->next = seed;
  seed->prev = route->nodes;
  seed->next = new_depot(route->pb);
  route->tail = seed->next;
  route->tail->prev = seed;
  route->len = ONE_CUSTOMER;  // includes the opening and closing depot
  route->load = seed->demand;
  route->workers = workers;
  calc_ests(route, route->nodes, workers);
  calc_lsts(route, route->tail, workers);
  return route;
}


//! Add one or more nodes after a given node on the specified route.
//! Do not check if the insertion is feasible.
//! Update the ests and lsts.
//! The added nodes are have to be removed from other routes before!
void add_nodes(Route* r, Node* first, Node* last, Node* after) {
  add_nodes_noupdate(r, first, last, after);
  calc_ests(r, first, r->workers);
  calc_lsts(r, last, r->workers);
}


//! Add one or more nodes after a given node on the specified route.
//! Do not check if the insertion is feasible.
//! Do not update the ests and lsts.
//! The added nodes are have to be removed from other routes before!
inline void add_nodes_noupdate(Route* r, Node* first, Node* last, Node* after) {
  Node* n = first;
  do {
    r->load += n->demand;
    r->len++;
    n = n->next;
  } while (n != last->next);
  first->prev = after;
  last->next = after->next;
  last->next->prev = last;
  after->next = first;
}



//! Modify the Insertion struct with the best insertion position
//! of the given node to the given route
//! if none of the positions is cheaper than the given insertion, it is left
//! unchanged
//! The insertion cost function is defined by Solomon's I1 heuristic
//! and includes both the distance and the time windows. See
//! Marius~M.~Solomon, "Algorithms for the Vehicle Routing and Scheduling
//! Problems with Time Window Constraints", Operations Research, Vol. 35,
//! No. 2, p. 257, 1987.
//! \return 1 if a new best insertion was found, otherwise 0
int calc_best_insertion(Route *route, Node *node, Insertion *ins) {
  int updated = 0;
  Node *after = route->nodes;
  double cost_dist = 0.0; double cost_time = 0.0; double cost = 0.0;
  double est_node = 0.0; double est_succ = 0.0;
  double **d = route->pb->c_m[0]; // distance
  double **c_m = route->pb->c_m[route->workers];
  double alpha = route->pb->cfg->alpha; double alpha2 = 1.0 - alpha;
  double mu = route->pb->cfg->mu;
  double lambda = route->pb->cfg->lambda;

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
      est_succ = max(after->next->est,
                      est_node + c_m[node->id][after->next->id]);
      cost_time = est_succ - after->next->aest;
    }
    cost = alpha * cost_dist + alpha2 * cost_time;
    // slight deviation from solomon: we minimize the "cost" instead of
    // maximizing the attractiveness, thus "cost - lambda * d_{0n}"
    cost = cost - lambda * d[0][node->id];
    if (cost < ins->cost) {
      ins->prev = (Insertion*) NULL;
      ins->next = (Insertion*) NULL;
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


//! Calculate and set the earliest start times.
//! \param workers number of workers that is used for calculating the ests
//! \param n first node that needs to be updated (update from n to tail)
void calc_ests(Route *route, Node *n, int workers) {
  #ifdef DEBUG
  if (!workers) {
    fprintf(stderr, "ERROR: calc_ests called with 0 workers\n");
    exit(EXIT_FAILURE);
  }
  #endif
  double **c_m = route->pb->c_m[workers];  // includes service time
  if (route->workers == workers) {  // calculate the actual values
    if (n == route->nodes) {  // if n is the opening depot
      n->aest = n->est;
      n = n->next;
    }
    while (n->next) {  //  aest is not relevant for the closing depot
                       //  as it is not used by insert*feasible
      n->aest = max(n->est, n->prev->aest + c_m[n->prev->id][n->id]);
      n = n->next;
    }
  } else {  // fill the cache
    if (n == route->nodes) {  // if n is the opening depot
      n->aest_cache = n->est;
      n = n->next;
    }
    while (n) {  // aest_cache is relevant for the closing depot as it is
                 // required by is_feasible_with
      n->aest_cache = max(n->est, n->prev->aest_cache +
        c_m[n->prev->id][n->id]);
      n = n->next;
    }
  }
}


//! Calculate and set the latest start times.
//! \param workers number of workers that is used for calculating the lsts
//! \param n last node that needs to be updated (update from n to head)
void calc_lsts(Route *route_ptr, Node *n, int workers) {
  double **c_m = route_ptr->pb->c_m[workers]; // includes service time
  if (n == route_ptr->tail) {  // if n is the closing depot
    n->alst = n->lst;
    n = n->prev;
  }
  while (n->prev) {
    n->alst = fmin(n->lst, n->next->alst - c_m[n->id][n->next->id]);
    n = n->prev;
  }
}


//! Calculate the total distance of the route.
double calc_length(Route* route) {
  double dist = 0.0;
  double** c_m = route->pb->c_m[0];
  Node* n = route->nodes->next; // first customer node
  while (n) {
    dist += c_m[n->prev->id][n->id];
    n = n->next;
  }
  return dist;
}


//! Clone a route and return a pointer to the clone.
//! If a route is cloned, all lists are regenerated to be different
//! objects.
Route* clone_route(Route* route) {
  Node *n = route->nodes;
  Route *clone = (Route *) s_malloc(sizeof(Route));
  clone->pb = route->pb;
  clone->id = route->id;
  clone->depot_id = route->depot_id;
  clone->len = route->len;
  clone->load = route->load;
  clone->workers = route->workers;
  clone->nodes = clone_node(n);
  clone->tail = clone->nodes;
  n = n->next;
  while (n) {
    clone->tail->next = clone_node(n);
    clone->tail->next->prev = clone->tail;
    clone->tail = clone->tail->next;
    n = n->next;
  }
  return clone;
}


//! "Destructor".
//! Free the memory of the given route and all allocated members.
void free_route(Route *route) {
  Node *n = route->nodes->next;
  while (n) {
    free(n->prev);
    n = n->next;
  }
  free(route->tail);
  free(route);
}


//! Return the best insertion of Node n on Route r.
//! Memory for the returned insertion is allocated and needs to be freed
//! by the caller.
//! If no insertion is feasible, the NULL pointer is returned.
//! The returned insertion structure can be used in doubly linked lists.
Insertion* get_best_insertion(Route* r, Node* n) {
  Insertion* ins = (Insertion*) NULL;
  if (r->pb->capacity < r->load + n->demand) return ins;
  double alpha = r->pb->cfg->alpha, alpha2 = 1.0 - alpha;
  double** c_m = r->pb->c_m[r->workers];
  double** d = r->pb->c_m[0];  // distance matrix
  double mu = r->pb->cfg->mu;
  double lambda = r->pb->cfg->lambda;
  Node* after = r->nodes;
  while (after != r->tail) {
    if (!can_insert_one(r, n, after)) {
      after = after->next;
      continue;
    }
    double cost = d[after->id][n->id] + d[n->id][after->next->id] -
                  mu * d[after->id][after->next->id];  // distance
    if (alpha2) {
      cost *= alpha;
      double est_node = max(n->est, after->aest + c_m[after->id][n->id]);
      double est_succ = max(after->next->aest, est_node +
                        c_m[n->id][after->next->id]);
      cost = alpha2 * (est_succ - after->next->aest);
    }
    double attract = lambda * d[DEPOT][n->id] - cost;
    if (attract < 0.0)
      attract = MIN_DELTA;
    if (!ins) {
      ins = (Insertion *) s_malloc(sizeof(Insertion));
      ins->attractiveness = -INFINITY;
    }
    if (attract > ins->attractiveness) {
      *ins = (Insertion) {.after = after, .attractiveness = attract,
        .cost = cost, .node = n, .target = r,
        .next = (Insertion*) NULL, .prev = (Insertion*) NULL};
    }
    after = after->next;
  }
  return ins;
}


//! Initialize or reset an insertion list.
void init_insertion_list(Insertion_List* il, long max_size) {
  if (!max_size) max_size = LONG_MAX;  // not limited
  il->head = (Insertion*) NULL;
  il->tail = (Insertion*) NULL;
  il->size = 0;
  il->max_size = max_size;
}


//! Return 1 if the route is feasible, otherwise 0.
//! This function should only be used to assure that a route is feasible
//! at the end of the overall computation. The known values for the
//! earliest start times etc are not used in order to assure there are no bugs.
int is_feasible(Route* r) {
  double **c_m = r->pb->c_m[r->workers];
  Node *n = r->nodes->next;
  double load = 0.0;
  double est = r->nodes->est;
  while (n) {
    load += n->demand;
    est = max(n->est, est + c_m[n->prev->id][n->id]);
    if (est > n->lst) {
      fprintf(stderr, "time window collision at node %d\n", n->id);
      print_route(stderr, r);
      return 0;
    }
    n = n->next;
  }
  if (load > r->pb->capacity) {
    fprintf(stderr, "route exceeds its capacity (%f/%u)\n", r->load,
            r->pb->capacity);
    print_route(stderr, r);
    return 0;
  }
  return 1;
}


//! Return True if the route is feasible in terms of all time windows.
//! The check is performed for the given numbers of workers in order
//! to see if that number can be used instead of the current number.
int is_feasible_with(Route *route, int workers) {
  Node *n = route->nodes->next;
  if (route->workers == workers) {
    return 1;
  }
  calc_ests(route, route->nodes, workers); // fill the cache
  while (n) {
    if (n->aest_cache > n->lst) {
      return 0;
    }
    n = n->next;
  }
  return 1;
}


//! Return a pointer to a randomly selected insertion.
//! If use_weights is set, return a pointer to an insertion selected by a
//! weighted roulette wheel.
//! The insertion is not removed from the list of insertions which is supposed
//! to be done by route::remove_invalid_insertions or
//! route::reset_insertion_list.
//! Important: the roulette wheel is coded such that all attractivenesses have
//! to be positive!
//! \param il Doubly linked list of insertions.
Insertion *pick_insertion(Insertion_List* il, int use_weights) {
  Insertion *ins = il->head;
  if (!ins) return ins;
  if (use_weights) {
    double cum_attractiveness = 0.0;
    while (ins) {
      cum_attractiveness += ins->attractiveness;
      ins = ins->next;
    }
    ins = il->head;
    double threshold = drand48() * cum_attractiveness;
    while (ins) {
      cum_attractiveness -= ins->attractiveness;
      if (threshold >= cum_attractiveness) return ins;
      ins = ins->next;
    }
    fprintf(stderr, "ERROR: pick_insertion: no insertion picked\n");
    fprintf(stderr, "ERROR: are there negative attractivenesses?");
    exit(EXIT_FAILURE);
  } else {
    int index = (int) (lrand48() % il->size);
    while (index--)
      ins = ins->next;
    return ins;
  }
}


// TODO: document & compare to _aco_
Insertion* pick_insertion_from_array(Insertion insertions[],
                                     int num_insertions) {
  double cum_attractiveness = 0.0;
  Insertion* ins = (Insertion*) NULL;
  double threshold = 0.0;

  for (int i = 0; i < num_insertions; ++i) {
    if (isinf(insertions[i].attractiveness)) continue;
    cum_attractiveness += insertions[i].attractiveness;
  }
  threshold = drand48() * cum_attractiveness;
  for (int i = 0; i < num_insertions; ++i) {
    if (isinf(insertions[i].attractiveness)) continue;
    cum_attractiveness -= insertions[i].attractiveness;
    if (threshold >= cum_attractiveness) {
      ins = &insertions[i];
      break;
    }
  }
  return ins;
}


//! Print a representation of the given route.
void print_route(FILE* stream, Route* route)
{
  Node *n = route->nodes->next;
  fprintf(stream, "[%d", route->nodes->id);
  while (n) {
    fprintf(stream, ", %3d", n->id);
    n = n->next;
  }
  fprintf(stream, "]: workers=%d, load=%6.2f, length=%.2f\n",
    route->workers, route->load, calc_length(route));
}


//! Remove unnecessary service workers from the given route.
int reduce_service_workers(Route *route) {
  int reduced = 0;
  int workers = route->workers - 1;
  Node *n = route->nodes;
  while (workers >= 1 && is_feasible_with(route, workers)) {
    route->workers = workers;
    n = route->nodes;
    while (n) {
      n->aest = n->aest_cache;
      n = n->next;
    }
    workers--;
    reduced = 1;
  }
  if (reduced) {
    calc_lsts(route, route->tail, route->workers);
  }
  return reduced;
}


// Return a pointer to the given insertion list after removing all invalid
// insertions.
// Invalid insertions occur after an insertion was performed - all insertions
// containing the inserted node as well as all insertions to the updated route
// need to be removed.
// All invalid insertions (including ins!) are removed from the list and freed.
// ins is the insertion that was performed
Insertion* remove_invalid_insertions(Insertion *old, Insertion *ins) {
  #ifdef DEBUG
  if (!old) {
    printf("remove_invalid_insertions: old == NULL\n");
    return (Insertion *) NULL;
  } else if (old->prev != NULL) {
    printf("remove_invalid_insertions: old->prev != NULL\n");
    exit(EXIT_FAILURE);
  }
  #endif // DEBUG
  Route *r = ins->target;
  Node *n = ins->node;
  Insertion *new = old;
  Insertion *temp = (Insertion *) NULL;
  while (old->next) {
    if (old->target == r || old->node == n) {
      if (old->prev)
        old->prev->next = old->next;
      else
        new = old->next; // removed the first
      old->next->prev = old->prev;
      temp = old;
      old = old->next;
      free(temp);
    } else
      old = old->next;
  }
  if (old->target == r || old->node == n) {
    if (old->prev)
      old->prev->next = (Insertion *) NULL; // end of the list
    else
      new = (Insertion *) NULL; // removed all elements
    free(old);
  }
  return new;
}


//! Remove one or more nodes from the given route.
//! Update the ests and lsts.
void remove_nodes(Route* r, Node* first, Node* last) {
  Node* prev = first->prev;
  remove_nodes_noupdate(r, first, last);
  calc_ests(r, prev->next, r->workers);
  calc_lsts(r, prev, r->workers);
}


//! Remove consecutive nodes and a worker from the given route.
//! Must not be run unless move_reduces_workers was run with the same
//! arguments before.
//! \param num_workers Number of workers to remove.
void remove_nodes_and_workers(Route* route, Node* first, Node* last,
                              int num_workers) {
  Node* n = first;
  do {
    route->load -= n->demand;
    route->len--;
    n = n->next;
  } while (n != last->next);
  first->prev->next = last->next;
  last->next->prev = first->prev;
  last->next = (Node *) NULL;
  first->prev = (Node *) NULL;
  n = route->nodes;
  while (n) {
    n->aest = n->aest_cache;
    n = n->next;
  }
  route->workers -= num_workers;
  calc_lsts(route, route->tail, route->workers);
}


//! Remove one or more nodes from the given route.
//! Do not update the ests and lsts.
inline void remove_nodes_noupdate(Route* r, Node* first, Node* last) {
  Node* n = first;
  do {
    r->load -= n->demand;
    r->len--;
    n = n->next;
  } while (n != last->next);
  first->prev->next = last->next;
  last->next->prev = first->prev;
  last->next = (Node *) NULL;
  first->prev = (Node *) NULL;
}


//! Reset the given insertion list to its initial values.
//! If it contained any insertions, free them.
void reset_insertion_list(Insertion_List* il) {
  if (il->head)
    free_insertions(il->head);
  init_insertion_list(il, il->max_size);
}


//! Swap n1 and n2 and update r1 and r2 accordingly.
//! No checks are performed.
void swap(Route* r1, Route* r2, Node* n1, Node* n2) {
  Node* temp = n1->prev;
  r1->load += n2->demand - n1->demand;
  r2->load += n1->demand - n2->demand;

  n1->prev = n2->prev;
  n2->prev = temp;
  temp = n1->next;
  n1->next = n2->next;
  n2->next = temp;

  n1->prev->next = n1;
  n1->next->prev = n1;
  n2->prev->next = n2;
  n2->next->prev = n2;

  n1->aest = n1->aest_cache;
  n1->next->aest = n1->next->aest_cache;  // next node is already updated
  n2->aest = n2->aest_cache;
  n2->next->aest = n2->next->aest_cache;
  if (n1->next->next && n1->next->next->next)  // don't update the closing depot
    calc_ests(r2, n1->next->next, r2->workers);
  if (n2->next->next && n2->next->next->next)
    calc_ests(r1, n2->next->next, r1->workers);
  calc_lsts(r2, n1, r2->workers);
  calc_lsts(r1, n2, r1->workers);
}


// only for debugging
void print_insertion_list(Insertion *ins) {
  if (!ins) {
    printf("no insertions\n");
    return;
  }
  if (ins->prev == (Insertion *) NULL)
    printf("NULL<-");
  else printf("ERROR - insertion list's head doesn't point to NULL\n");
  while (ins) {
    printf("%p:%d->", ins->target, ins->node->id);
    ins = ins->next;
  }
  printf("NULL\n");
}


//! Add `ins` to the insertion list and remove the worst element if needed.
//! Ins has to be added to the correct position (sorted by attractiveness,
//! where the head is the most attractive element. If
//! the size would grow
//! beyond il->max_size, the worst element has to be freed and the pointers
//! need to be updated accordingly.
//! \return 1 if `ins` was inserted, otherwise 0.
// TODO: rewrite properly and cleanly
int update_insertion_list(Insertion_List* il, Insertion* ins) {
  if (!il->size) {  // the first element
    il->head = ins;
    il->tail = ins;
    il->size++;
    return 1;
  }
  if (il->max_size == 1) {
    if (il->head->attractiveness > ins->attractiveness) {
      free(ins);
      return 0;
    } else {
      free(il->head);
      il->head = ins;
      il->tail = ins;
      return 1;
    }
  }
  if (il->size < il->max_size) {
    if (il->head->attractiveness < ins->attractiveness) {  // ins is head
      ins->next = il->head;
      il->head->prev = ins;
      il->head = ins;
      il->size++;
      return 1;
    }
    Insertion* temp = il->tail;
    while (temp->attractiveness < ins->attractiveness)
      temp = temp->prev;
    if (temp == il->tail) {
      il->tail = ins;
    } else {
      ins->next = temp->next;
      temp->next->prev = ins;
    }
    temp->next = ins;
    ins->prev = temp;
    il->size++;
  } else {
    if (il->tail->attractiveness > ins->attractiveness) {
      free(ins);
      return 0;
    } else if (il->head->attractiveness < ins->attractiveness) {
      ins->next = il->head;
      il->head->prev = ins;
      il->head = ins;
    } else {
      Insertion* temp = il->tail->prev;
      while (temp->attractiveness < ins->attractiveness)
        temp = temp->prev;
      ins->next = temp->next;
      ins->next->prev = ins;
      ins->prev = temp;
      temp->next = ins;
    }
    Insertion* temp = il->tail->prev;
    free(il->tail);
    il->tail = temp;
    il->tail->next = (Insertion*) NULL;
  }
  return 1;
}
