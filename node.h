/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef NODE_H
#define NODE_H

#include "common.h"

typedef struct supernode Supernode;  // TODO: scheduled for removal

struct node {
  int id;
  double x;
  double y;
  double demand;
  double est;
  double lst;
  double service_time;
  double aest;  // actual earliest starting time
  double alst;  // actual latest starting time
  double aest_cache;  // to cache aest values for more or less workers
  double alst_cache;
  Node *prev;
  Node *next;
};

// contains two or more nodes; for efficient feasibility tests when adding
// nodes to a route
struct supernode {  // TODO: scheduled for removal
  Node *nodes;  // contains in_x and in_y
  Node *tail;  // contains out_x and out_y
  int count;
  double est;
  double lst;
  double cum_time;  // includes all service times and internal travel time
};

Node *clone_node(Node *);
void print_node(Node *);

static inline double sum_demands(Node* first, Node* last) {
  double demand = first->demand;
  while (first != last) {
    first = first->next;
    demand += first->demand;
  }
  return demand;
}

#endif
