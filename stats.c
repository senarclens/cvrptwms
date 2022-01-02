/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "node.h"
#include "route.h"
#include "wrappers.h"
#include "stats.h"


#ifndef STATS
// avoid losing performance if no stats are needed
// this approach avoids cluttering the code using stats with ifdefs
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
inline void free_stats(Stats* stats, size_t dim) {}
inline Stats* init_stats(size_t dim) {return (Stats*) NULL;}
inline void document_move(Stats* s, Route* source, Node* n,
                          Route* target, Node* pred, int delta_trucks,
                          int delta_workers, double delta_dist) {}
inline void write_stats(Stats* stats, char* fname) {}
#pragma GCC diagnostic pop
#else // #ifndef STATS


//! "Constructor." Initialize the stats and returns a pointer to it.
Stats* init_stats(size_t dim) {
  Stats* stats = (Stats*) s_malloc(sizeof(Stats));
  stats->dim = dim;
  stats->attempted_move1 = init_int_vector(dim, 0);
  stats->performed_move1 = init_int_vector(dim, 0);
  stats->attempted_move2 = init_int_matrix(dim, 0);
  stats->performed_move2 = init_int_matrix(dim, 0);
  stats->moves = (PastMove*) NULL;
  stats->moves_tail = (PastMove*) NULL;
  return stats;
}


//! Document a single move by adding it to the stats object.
//! The move is added to the tail of the list of moves as well as to the array
//! of attempted and performed moves. Also, the count of performed moves is
//! incremented.
//! \param n The node before it is moved.
//! \param pred The predecessor on the target route before the move.
void document_move(Stats* s, Route* source, Node* n,
                   Route* target, Node* pred,
                   int delta_trucks, int delta_workers, double delta_dist) {
  s->performed_move1[n->id]++;
  PastMove* m = (PastMove*) s_malloc(sizeof(PastMove));
  m->performed = s->performed_move1[n->id];
  m->node_id = n->id;
  m->old_route_id = source->id;
  m->old_pred_id = n->prev->id;
  m->old_succ_id = n->next->id;
  m->new_route_id = target->id;
  m->new_pred_id = pred->id;
  m->new_succ_id = pred->next->id;
  m->delta_trucks = delta_trucks;
  m->delta_workers = delta_workers;
  m->delta_dist = delta_dist;
  m->next = (PastMove*) NULL;
  if (!s->moves) {
    s->moves = m;
    s->moves_tail = m;
  } else {
    s->moves_tail->next = m;
    s->moves_tail = m;
  }
}


//! Free all resources acquired by the stats.
void free_stats(Stats* stats, size_t dim) {
  free(stats->attempted_move1);
  free(stats->performed_move1);
  free_int_matrix(stats->attempted_move2, dim);
  free_int_matrix(stats->performed_move2, dim);
  while (stats->moves) {
    PastMove* next = stats->moves->next;
    free(stats->moves);
    stats->moves = next;
  }
  free(stats);
}


//! Write stats to file.
void write_stats(Stats* stats, char* fname) {
  FILE *fp = fopen(fname, "w");
  int cnt = 0;
  if (!fp) {
    fprintf(stderr, "write_stats: could not open \"%s\" for writing)\n", fname);
    return;
  }
  fprintf(fp, "move1 (detailed)\n================\n");
  PastMove* m = stats->moves;
  while (m) {
    cnt++;
    fprintf(fp, "%3d|%3d | %d:%3d->%3d->%3d => %d:%3d--%3d | %2d %2d %9.3f\n",
            cnt, m->performed,
            m->old_route_id, m->old_pred_id, m->node_id, m->old_succ_id,
            m->new_route_id, m->new_pred_id, m->new_succ_id,
            m->delta_trucks, m->delta_workers, m->delta_dist);
    m = m->next;
  }
  fprintf(fp, "\nmove1\n=====\n");
  for (size_t i = 0; i < stats->dim; ++i) {
    if (stats->performed_move1[i])
      fprintf(fp, "%lu: %d / %d\n", i, stats->performed_move1[i],
              stats->attempted_move1[i]);
  }
  fprintf(fp, "\nmove2\n=====\n");
  for (size_t i = 0; i < stats->dim; ++i) {
    for (size_t j = 0; j < stats->dim; ++j) {
      if (stats->performed_move2[i][j])
        fprintf(fp, "%lu->%lu: %d / %d\n", i, j, stats->performed_move2[i][j],
                stats->attempted_move2[i][j]);
    }
  }
  printf("wrote %s...\n", fname);
  fclose(fp);
}
#endif
