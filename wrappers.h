/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef WRAPPERS_H
#define WRAPPERS_H

#include <stdlib.h>

#define s_malloc(size) safe_malloc_ (size, __FILE__, __LINE__)

void *safe_malloc_(size_t size, const char* filename, int line);

#endif
