/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef ROUTE_H
#define ROUTE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "node.h"  // for statically inlined function calls
#include "problemreader.h"  // for statically inlined function calls

static const double MIN_COST = 0.001;  // TODO: can't be 0 (b/c of div); ideas?

//! Describes the number of customers in a route.
//! This is required for clarity and readability as the route's len includes
//! the starting and ending depots.
enum RouteLength {
  EMPTY = 2,  //!< No customers (just the opening and closing depot).
  ONE_CUSTOMER = 3,
  TWO_CUSTOMERS,
};

enum Weights {
  NO_WEIGHTS,
  USE_WEIGHTS,
};

//! \struct route
//! Stores a single route (which corresponds to a single truck).
//! The nodes in the route are stored in a doubly linked list.
//! The upsides of this approach are
//! <ul>
//!   <li>the implementation is easier than with arrays</li>
//!   <li>updating of ests and lsts is safer & clearer</li>
//!   <li>adding/ removing of nodes is faster & safer</li>
//!   <li>initialization of routes becomes faster</li>
//! </ul>
struct route {
  int id;  //!< Starts with 0 and is unique unless the route is cloned.
  int depot_id;  //!< Starting with num_nodes (+0 for the first route).
  Node *nodes;  //!< Doubly linked list of nodes.
  Node *tail;  //!< Pointer to the nodelist's tail.
  int len;  //!< The number of nodes including the depot at the start and end
            //!< (the total distance is not stored).
  double load;  //!< The truck's (route's) current load.
  int workers;  //!< The number of workers currently assigned to this route.
  Problem *pb;
};

struct insertion {
  Route* target;
  Node* node;  //!< Node to be inserted.
  Node* after;  //!< Add node after this node.
  double cost;
  double attractiveness;
  Insertion* next;  //!< Next insertion in insertion list.
  Insertion* prev;  //!< Previous insertion in insertion list.
};

//! A list of insertions that can be sorted.
struct insertion_list {
  Insertion* head;  //!< Head of the insertion list.
  Insertion* tail;  //!< Tail of the insertion list.
  long size;
  long max_size;
};

Route* new_route(Solution*, Node*, int workers);
void add_nodes(Route*, Node* first, Node* last, Node* after);
extern void add_nodes_noupdate(Route*, Node* first, Node* last, Node* after);
int calc_best_insertion(Route*, Node*, Insertion*);
void calc_ests(Route*, Node*, int workers);
void calc_lsts(Route*, Node*, int workers);
double calc_length(Route*);
Route *clone_route(Route* route);
void free_route(Route* route);
Insertion* get_best_insertion(Route*, Node*);
void init_insertion_list(Insertion_List* il, long max_size);
int is_feasible(Route*);
int is_feasible_with(Route*, int workers);
// int move_best_node(Route* source, Route* target, int state);
Insertion* pick_insertion(Insertion_List* il, int use_weights);
Insertion* pick_insertion_from_array(Insertion[], int num_insertions);
void print_route(FILE* stream, Route*);
int reduce_service_workers(Route*);
Insertion *remove_invalid_insertions(Insertion* old, Insertion* ins)
  __attribute__ ((warn_unused_result));
void remove_nodes(Route*, Node* first, Node* last);
void remove_nodes_and_workers(Route*, Node* first, Node* last, int);
extern void remove_nodes_noupdate(Route*, Node* first, Node* last);
void reset_insertion_list(Insertion_List* il);
void swap(Route* r1, Route* r2, Node* n1, Node* n2);
int update_insertion_list(Insertion_List* il, Insertion* ins);




//! Return True if the suggested insertion is feasible.
//! An insertion is feasible if there is no collision in the earliest and latest
//! start times. The approach used in this function is faster than the more
//! straight-forward approach of using max(est, pred->aest + c_m[...]).
//! The total load is not checked by this function.
static inline bool can_insert_one(Route *route, Node *n, Node *pred) {
  #ifdef DEBUG
  if (!pred) {
    fprintf(stderr, "ERROR: can_insert_one: pred == NULL\n");
    exit(EXIT_FAILURE);
  }
  if (pred == route->tail) {
    fprintf(stderr,
            "ERROR: can_insert_one: can't insert after closing depot\n");
    exit(EXIT_FAILURE);
  }
  #endif // DEBUG
  double **c_m = route->pb->c_m[route->workers];
  double earliest_arrival = pred->aest + c_m[pred->id][n->id];
  double latest_arrival = pred->next->alst - c_m[n->id][pred->next->id];
  return (earliest_arrival <= n->lst) && (latest_arrival >= n->est) &&
    (earliest_arrival <= latest_arrival);
}


// TODO name properly and move to proper location;
// profile in comparison to insert_feasible
#include <math.h>
static inline bool can_insert(Route* target, Node* first, Node* last,
                             Node* after) {
  double **c_m = target->pb->c_m[target->workers];  // driving + service time
  first->aest_cache = max(after->aest + c_m[after->id][first->id], first->est);
  if (first->aest_cache > first->lst) return 0;
  while (first != last) {
    Node* next = first->next;
    next->aest_cache = max(first->aest_cache + c_m[first->id][next->id],
                            next->est);
    if (next->aest_cache > next->lst) return 0;
    first = next;
  }
  return ((last->aest_cache + c_m[last->id][after->next->id]) <=
          after->next->alst);
}


// only for debugging
void print_insertion_list(Insertion *ins);


#endif
