/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <stddef.h>
#include <stdio.h>

#include "common.h"
#include "wrappers.h"


///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////

//! Free the memory of the given square double matrix of the given dimension.
void free_double_matrix(double **matrix, size_t dim) {
  for (size_t i = 0; i < dim; ++i) {
    free(matrix[i]);
  }
  free(matrix);
}


//! Free the memory of the given square int matrix of the given dimension.
void free_int_matrix(int **matrix, size_t dim) {
  for (size_t i = 0; i < dim; ++i) {
    free(matrix[i]);
  }
  free(matrix);
}


//! Free the memory of the given square unsigned long matrix.
void free_unsigned_long_matrix(long unsigned int** matrix, size_t rows) {
  for (size_t i = 0; i < rows; ++i) {
    free(matrix[i]);
  }
  free(matrix);
}


//! Allocate memory for a double matrix and return pointer to it.
double** init_double_matrix(size_t dim, double val) {
  double** matrix = (double**) s_malloc(sizeof(double*) * dim);
  for (size_t i = 0; i < dim; ++i) {
    matrix[i] = (double*) s_malloc(sizeof(double) * dim);
    for (size_t j = 0; j < dim; ++j) {
      matrix[i][j] = val;
    }
  }
  return matrix;
}


//! Allocate memory for a double vector and return pointer to it.
double* init_double_vector(size_t dim, double val) {
  double* vector = (double*) s_malloc(sizeof(double) * dim);
  for (size_t i = 0; i < dim; ++i) {
    vector[i] = val;
  }
  return vector;
}


//! Allocate memory for an int matrix and return pointer to it.
int** init_int_matrix(size_t dim, int val) {
  int** matrix = (int**) s_malloc(sizeof(int*) * dim);
  for (size_t i = 0; i < dim; ++i) {
    matrix[i] = (int*) s_malloc(sizeof(int) * dim);
    for (size_t j = 0; j < dim; ++j) {
      matrix[i][j] = val;
    }
  }
  return matrix;
}


//! Allocate memory for an int vector and return pointer to it.
int* init_int_vector(size_t dim, int val) {
  int* vector = (int*) s_malloc(sizeof(int) * dim);
  for (size_t i = 0; i < dim; ++i) {
    vector[i] = val;
  }
  return vector;
}


long unsigned int** init_unsigned_long_matrix(size_t rows,
                                              size_t cols,
                                              long unsigned int val) {
  unsigned long** matrix = (unsigned long**) s_malloc(sizeof(unsigned long*) *
                                                      rows);
  for (size_t i = 0; i < rows; ++i) {
    matrix[i] = (unsigned long*) s_malloc(sizeof(unsigned long) * cols);
    for (size_t j = 0; j < cols; ++j) {
      matrix[i][j] = val;
    }
  }
  return matrix;
}


// TODO: allow for printing 100% using an argument; make m x n
void print_double_matrix(int num, double** matrix, const char* name) {
  int i, j;
  printf ("%dx%d ", num, num);
  if (num > 10) {
    printf ("(truncated) ");
  }
  printf ("%s\n", name);
  for (i = 0; i < num; i++) {
    if (i == 5 && num > 13) {
      printf (" ... ");
    } else if (i > 5 && num - i > 5 && num > 13) {
      continue;
    } else {
      for (j = 0; j < num; j++) {
        if (j == 5 && num - j > 5) {
          printf ("... ");
        } else if (j > 5 && num - j > 5) {
          continue;
        } else {
          printf ("%4.5f ", matrix[i][j]);
        }
      }
    }
    printf ("\n");
  }
}


void set_double_matrix(double** matrix, size_t rows, size_t cols, double val) {
  for (size_t i = 0; i < rows; ++i) {
    for (size_t j = 0; j < cols; ++j) {
      matrix[i][j] = val;
    }
  }
}
