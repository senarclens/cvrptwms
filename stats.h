/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef STATS_H
#define STATS_H

#include <stddef.h>

#include "common.h"
#include "node.h"
#include "route.h"

//! Singly linked list documenting past moves for TS development.
struct past_move {
  int node_id;
  int performed;  //!< Current number of successful moves for this node.
  int old_route_id;
  int old_pred_id;
  int old_succ_id;
  int new_route_id;
  int new_pred_id;
  int new_succ_id;
  int delta_trucks;
  int delta_workers;
  double delta_dist;
  PastMove* next;
};

struct stats {
  size_t dim;  // dimension of the stored vectors and matrices
  int* attempted_move1;
  int* performed_move1;
  int** attempted_move2;
  int** performed_move2;
  PastMove* moves;
  PastMove* moves_tail;
};

void document_move(Stats* s, Route* source, Node* n, Route* target, Node* pred,
                   int delta_trucks, int delta_workers, double delta_dist);
void free_stats(Stats*, size_t dim);
Stats* init_stats(size_t dim);
void write_stats(Stats*, char* fname);

#endif