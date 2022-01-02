#include <stdio.h>
#include "wrappers.h"
#include "node.h"

///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////

//! Return pointer to a clone of the given node.
Node* clone_node(Node* node) {
  Node* clone = (Node*) s_malloc(sizeof(Node));
  clone->id = node->id;
  clone->x = node->x;
  clone->y = node->y;
  clone->demand = node->demand;
  clone->est = node->est;
  clone->lst = node->lst;
  clone->aest = node->aest;
  clone->alst = node->alst;
  clone->service_time = node->service_time;
  clone->prev = (Node *) NULL;  // the clone must not point to the original list
  clone->next = (Node *) NULL;
  clone->aest_cache = -1.0;
  clone->alst_cache = -1.0;
  return clone;
}


//! Print the given node to stdout.
void print_node(Node *n) {
  printf("Node %3d: (%4.1f/%4.1f) d=%4.1f %6.1f - %6.1f st=%2.0f\n", n->id,
         n->x, n->y, n->demand, n->est, n->lst, n->service_time);
}

