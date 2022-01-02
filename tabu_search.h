/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef TABU_SEARCH_H
#define TABU_SEARCH_H

#include "common.h"

//! Stores all data relevant for different tabu criteria.
struct tabulist {
  int active;  //!< This flag indicates whether or not to use the tabu system.
  //! Counts the iterations. One local search move corresponds to one
  //! iteration. Needs to be incremented on every applied local search operator
  //! before using it to calculate any tabu times blocking this move.
  unsigned long iteration;
  //! Stores which node is (not) allowed to be moved on which route.
  //! This is done by storing an iteration number for each node/ route
  //! combination. If the stored number is strictly greater than the iteration
  //! number (not greater or equal),
  //! the node may not be moved to that route.
  //! Whenever a node is moved to a route, the corresponding value in this
  //! matrix is set to `iteration` + `nodes_routes_tabutime` (where the
  //! iteration counter has to be incremented before this is done).
  unsigned long** nodes_routes;
  //! Number of iterations that a node is not allowed back on a route.
  unsigned long nodes_routes_tabutime;
};

int is_move_tabu(Tabulist* tl, Move* m);
Tabulist* new_tabulist(Problem*);
void free_tabulist(Tabulist* tl, size_t dim);
void solve_ts(Problem*, int workers);
void update_tabulist_move(Tabulist* tl, Move* m);

#endif  // TABU_SEARCH_H
