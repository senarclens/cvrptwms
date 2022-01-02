/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef CACHED_GRASP_H
#define CACHED_GRASP_H

#include "common.h"

void solve_cached_grasp(Problem*, int workers);
extern "C" void c_solve_cached_grasp(Problem* pb, int workers);

#endif // CACHED_GRASP_H
