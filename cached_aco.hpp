/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef CACHED_ACO_H
#define CACHED_ACO_H

#include "common.h"

// Insertion* aco_pick_insertion(Insertion[], int num_insertions, double min_cost);
void solve_cached_aco(Problem*, int workers);
extern "C" void c_solve_cached_aco(Problem* pb, int workers);

#endif // CACHED_ACO_H
