/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <libgen.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "node.h"
#include "stats.h"
#include "solution.h"
#include "tabu_search.h"
#include "wrappers.h"
#include "problemreader.h"

static const int SKIPROWS = 9;
static const int CAPACITY_LINE = 5;


///////////////////////////////////////////////////////////////////////////////
// File Scope (Static) Functions                                             //
///////////////////////////////////////////////////////////////////////////////
static void adapt_service_times(int num, Node **nodes, double **d,
                                Config *cfg_ptr);
static int get_node_count(FILE *fp);
static Node **get_nodes(size_t num, FILE *);
static double ***get_cost_matrix(int num, Node **nodes, Config *cfg_ptr);
static unsigned int get_truck_capacity(FILE *fp);


//! Adapt the service times according to Reimann et al. 2011.
static void adapt_service_times(int num, Node** nodes, double** d, Config*
cfg_ptr) {
  double service_rate = cfg_ptr->service_rate;
  double truck_velocity = cfg_ptr->truck_velocity;

  if (!cfg_ptr->adapt_service_times)
    return;
  for (int i = 1; i < num; ++i) {
    nodes[i]->service_time = fmin(service_rate * nodes[i]->demand,
                                  nodes[0]->lst -
                                  max(nodes[i]->est,
                                       d[nodes[0]->id][nodes[i]->id] /
                                       truck_velocity) -
                                  d[nodes[i]->id][nodes[0]->id] /
                                  truck_velocity);
  }
}


//! Return the number of nodes in the given problem.
static int get_node_count(FILE* fp) {
  rewind(fp);
  int nodeCount = 0;
  int col = 0;
  int skip = SKIPROWS;
  char line[100];
  char *sub_string;
  while (fgets(line, sizeof(line), fp) != NULL) {
    if (skip) {
      skip--;
      continue;
    }
    /* Extract first string */
    strtok(line, " ");
    col = 1;
    /* Extract remaining strings */
    while ( (sub_string=strtok(NULL, " ")) != NULL) {
      col++;
      if (col == 7) {
        nodeCount++;
        continue;
      }
    }
  }
  return nodeCount;
}


//! Return an array of pointers to the problem's nodes.
//! Also allocate memory for each of the problem's nodes.
static Node** get_nodes(size_t num, FILE* fp) {
  rewind(fp);
  Node *nodes = (Node *) s_malloc(sizeof(Node) * num);
  Node **node_ptrs = (Node **) s_malloc(sizeof(Node *) * num);
  int skip = SKIPROWS;
  int col = 0; // the first col gets index 0
  char line[100];
  char *sub_string;
  for (size_t i = 0; i < num; i++) {
    if (fgets(line, sizeof(line), fp) == NULL) return NULL;
    if (skip > 0) {
      skip--;
      i--;
      continue;
    }
    nodes[i].id = atoi(strtok(line, " "));
    col = 0;
    // Extract remaining strings
    while ((sub_string=strtok(NULL, " ")) != NULL) {
      col++;
      switch (col) {
        case 1:
          nodes[i].x = atof(sub_string);
          break;
        case 2:
          nodes[i].y = atof(sub_string);
          break;
        case 3:
          nodes[i].demand = atof(sub_string);
          break;
        case 4:
          nodes[i].est = atof(sub_string);
          break;
        case 5:
          nodes[i].lst = atof(sub_string);
          break;
        case 6:
          nodes[i].service_time = atof(sub_string);
          break;
      }
    }
    nodes[i].next = (Node *) NULL;
    nodes[i].prev = (Node *) NULL;
    nodes[i].aest = -1.0;
    nodes[i].alst = -1.0;
    nodes[i].aest_cache = -1.0;
    nodes[i].alst_cache = -1.0;
    node_ptrs[i] = &nodes[i];
  }
  return node_ptrs;
}


//! Return array of cost matrices.
//! Each of the matrices is calculated from the node's data.
//! \return [0] is the distance matrix.
//!         [1-...] are matrices of the distance plus the required service time
//!         in the source node given [1-...] workers.
static double ***get_cost_matrix(int num, Node **nodes, Config *cfg_ptr) {
  int workers = 0;
  int max_workers = (int) cfg_ptr->max_workers;
  double delta_x = 0.0;
  double delta_y = 0.0;
  double ***c_m = (double***) s_malloc((size_t) (1 + max_workers) *
  sizeof(double**));
  for (int i = 0; i <= max_workers; i++) {
    c_m[i] = (double**) s_malloc((size_t) num * sizeof(double*));
    for (int j = 0; j < num; j++) {
      c_m[i][j] = (double*) s_malloc((size_t) num * sizeof(double));
    }
  }
  for (int i = 0; i < num; i++) {
    for (int j = 0; j < num; j++) {
      if (i == j) {
        c_m[0][i][j] = 0.0;
        continue;
      }
      delta_x = (nodes[i]->x - nodes[j]->x) * (nodes[i]->x - nodes[j]->x);
      delta_y = (nodes[i]->y - nodes[j]->y) * (nodes[i]->y - nodes[j]->y);
      c_m[0][i][j] = sqrt(delta_x + delta_y);
    }
  }
  adapt_service_times(num, nodes, c_m[0], cfg_ptr);
  // add additional matrices for the total time (including service time)
  for (workers = 1; workers <= max_workers; workers++) {
    for (int i = 0; i < num; i++) {
      for (int j = 0; j < num; j++) {
        if (i == j) {
          c_m[workers][i][j] = 0.0; // irrel. => ignore service time
          continue;
        }
        c_m[workers][i][j] = c_m[0][i][j] +
        nodes[i]->service_time / (double) workers;
      }
    }
  }
  return c_m;
}


//! Return the truck's capacity.
static unsigned int get_truck_capacity(FILE* fp) {
  rewind(fp);
  unsigned capacity = 0;
  char line[100];
  int line_nr = 1;
  while (fgets(line, sizeof(line), fp) != NULL) {
    if (line_nr < CAPACITY_LINE) {
      line_nr++;
      continue;
    }
    sscanf(line, "%*u %u", &capacity);
    break;
  }
  return capacity;
}



///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////

//! Free the memory of the given VRPTWMS.
//! Do not free the config as it may be used by other problem instances.
//! The config has to be freed separately.
void free_problem(Problem* pb) {
  free(pb->nodes[0]);
  free(pb->nodes);
  for (int i = 0; i <= pb->cfg->max_workers; ++i) {
    free_double_matrix(pb->c_m[i], (size_t) pb->num_nodes);
  }
  free(pb->c_m);
  free(pb->name);
  free_solution(pb->sol);
  free_double_matrix(pb->pheromone, (size_t) (2 * pb->num_nodes - 1));
  free_stats(pb->stats, (size_t) pb->num_nodes);
  free_tabulist(pb->tl, (size_t) pb->num_nodes);
  free(pb);
}


//! Return the problem's name (the problem's filename without its extension).
char* get_name(const char* fname) {
  char* name;
  fname = basename((char*) fname);
  if ((name = malloc (strlen(fname) + 1)) == NULL)
    return NULL;
  strcpy (name, fname);
  char* dot = strrchr (name, '.');
  if (dot != NULL)
    *dot = '\0';
  return name;
}


//! Allocate memory for a problem instance and return a pointer to it.
Problem* get_problem(const char* fname, Config* cfg) {
  FILE *fp = fopen(fname, "r");
  if (!fp) {
    fprintf(stderr, "input file \"%s\" is ignored (not readable)\n", fname);
    return NULL;
  }
  Problem* pb = (Problem*) s_malloc(sizeof(Problem));
  pb->num_nodes = get_node_count(fp);
  pb->cfg = cfg;
  if (pb->cfg->ants_dynamic) pb->cfg->ants = pb->num_nodes - 1;
  pb->nodes = get_nodes((size_t) pb->num_nodes, fp);
  pb->c_m = get_cost_matrix(pb->num_nodes, pb->nodes, cfg);
  pb->capacity = get_truck_capacity(fp);
  pb->num_solutions = 0;
  pb->name = get_name(fname);
  pb->start_time = time((time_t*) NULL);
  pb->sol = new_solution(pb);
  pb->pheromone = init_double_matrix((size_t) ((2 * pb->num_nodes) - 1),
                                     cfg->initial_pheromone);
  pb->state = REDUCE_TRUCKS;
  pb->attempts = 0;
  pb->tl = new_tabulist(pb);
  fclose(fp);
  pb->stats = init_stats((size_t) pb->num_nodes);
  return pb;
}


//! Return pointer to a clone of the depot.
Node* new_depot(Problem *pb) {
  return clone_node(pb->nodes[DEPOT]);
}


//! Print the problem to stdout.
void print_problem(Problem *pb) {
  if (!pb) return;
  int i = 0;
  printf ("problem: %s\n", pb->name);
  printf ("truck capacity: %u\n", pb->capacity);
  printf ("%d nodes (including the depot)\n", pb->num_nodes);
  for (i = 0; i < pb->num_nodes; ++i) {
    print_node(pb->nodes[i]);
  }
  printf("\n");
  print_double_matrix(pb->num_nodes, pb->c_m[0], "cost matrix");
}

