/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef ACO_H
#define ACO_H

#include "common.h"

// TODO: move aco_pick_insertion to private when vrptwms::solve_solomon
// is updated; import of route becomes obsolete then :)
#include "route.h"
void aco_construct_routes(Solution* sol, int workers);
Insertion* aco_pick_insertion(Insertion[], int num_insertions, double min_cost);
void solve_aco(Problem*, int workers);
void solve_gaco(Problem*, int workers);
void update_pheromone(Problem*, Solution*);

#endif // ACO_H
