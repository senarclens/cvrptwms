/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef LOCAL_SEARCH_H
#define LOCAL_SEARCH_H

#include <float.h>
#include <stdbool.h>

#include "common.h"

static const int NON_IMPROVING = 0;
static const int IMPROVING = 1;

struct move {
  Route* source;
  Route* target;
  Node* first;
  Node* last;
  Node* after;  //!< Insert after this node.
  int delta_trucks;  //!< Positive delta implies savings. Can be 0 or 1.
  int delta_workers;  //!< Positive delta implies savings.
  double delta_dist;  //!< Positive delta implies savings.
  int improving;  //!< Only allow improving moves.
  Move* next;  //!< Doubly linked list allowing to keep sorted list of moves.
  Move* prev;
};


int brute_reduce_trucks(Solution**);
Solution* do_ls(Solution*) __attribute__ ((warn_unused_result));
int move_all(Solution*, int state);
int move_all_best(Solution*, int state);
void perform_move(Solution*, Move*);
void reduce_distance(Solution*);
Solution* reduce_trucks(Solution*)
  __attribute__ ((warn_unused_result));
void reduce_workers(Solution*);
int swap_all(Solution* sol);
int update_move(Move* m, Route* source, Route* target, int state, int len);




//! Init (or reset) the given move.
//! If improving is not set, allow worsening moves as well.
static inline void init_move(Move* m, bool improving) {
  if (improving)
    *m = (Move) {.source = (Route*) NULL, .target = (Route*) NULL,
      .first = (Node*) NULL, .last = (Node*) NULL, .after = (Node*) NULL,
      .delta_trucks = 0, .delta_workers = 0, .delta_dist = 0.0,
      .improving = improving, .next = (Move*) NULL, .prev = (Move*) NULL};
  else
    *m = (Move) {.source = (Route*) NULL, .target = (Route*) NULL,
      .first = (Node*) NULL, .last = (Node*) NULL, .after = (Node*) NULL,
      .delta_trucks = 0, .delta_workers = 0, .delta_dist = -DBL_MAX,
      .improving = improving, .next = (Move*) NULL, .prev = (Move*) NULL};
}

#endif
