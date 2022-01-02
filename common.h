/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef COMMON_H
#define COMMON_H

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600  // Request non-standard functions (drand48 et al.)
#endif

#include <stddef.h>

static const double MIN_DELTA = 1e-13;  // avoid infinite loop
static const int DEPOT = 0;  // the depot's node id
static const int UNLIMITED = 0;

typedef struct config Config;
typedef struct insertion Insertion;
typedef struct insertion_list Insertion_List;
typedef struct move Move;
typedef struct node Node;
typedef struct past_move PastMove;
typedef struct resultlist Resultlist;
typedef struct route Route;
typedef struct problem Problem;
typedef struct solution Solution;
typedef struct stats Stats;
typedef struct tabulist Tabulist;

void free_double_matrix(double** matrix, size_t dim);
void free_int_matrix(int** matrix, size_t dim);
void free_unsigned_long_matrix(unsigned long** matrix, size_t rows);
double** init_double_matrix(size_t dim, double val);
double* init_double_vector(size_t dim, double val);
int** init_int_matrix(size_t dim, int val);
int* init_int_vector(size_t dim, int val);
unsigned long** init_unsigned_long_matrix(size_t rows, size_t cols,
                                          unsigned long val);
void print_double_matrix(int num, double** matrix, const char* name);
void set_double_matrix(double** matrix, size_t rows, size_t cols, double val);

static inline double max(double x, double y) {
  return x > y ? x : y;
}

#endif
