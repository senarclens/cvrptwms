/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef GRASP_H
#define GRASP_H

#include "common.h"

void grasp_construct_routes(Solution* sol, int workers);
void solve_grasp(Problem*, int workers);

#endif // GRASP_H
