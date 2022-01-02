/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <confuse.h>  // external config file parser
#include <stdio.h>
#include "common.h"

// Textual configuration values that are not set will default to NOT_SET;
#define NOT_SET "not set"


//! all enums should start at 0 and have to be consecutive
enum Metaheuristic {
  NO_METAHEURISTIC = 0,
  ACO,
  CACHED_ACO,
  CACHED_GRASP,
  GACO,
  GRASP,
  TS,
  VNS,
  FIRST_METAHEURISTIC = NO_METAHEURISTIC,
  LAST_METAHEURISTIC = VNS,
};
extern const char* METAHEURISTICS[];

enum StartHeuristic {
  SOLOMON,
  SOLOMON_MR,  // Marc Reimann style implementation of stochastic solomon
//  ADAPTED,  //  TODO: implement
//  TARGETSIZE,  //  TODO: implement
  PARALLEL,
  FIRST_STARTHEURISTIC = SOLOMON,
  LAST_STARTHEURISTIC = PARALLEL,
};
extern const char* START_HEURISTICS[];

enum OutputFormat {
  HUMAN,
  CSV,
  FIRST_OUTPUTFORMAT = HUMAN,
  LAST_OUTPUTFORMAT = CSV,
};
extern const char* OUTPUT_FORMATS[];

enum VerbosityLevel {
  MIN_VERBOSITY,  // only print solution summary
  BASIC_VERBOSITY,  // print detailed solution, seed, ...
  BASIC_DEBUG,
  // FLAGS
  DEBUG_FLAGS = 9,
  DEBUG_CACHE,

  FULL_DEBUG = 99,  // verbose debug output; enables all flags
};

struct config {
  cfg_bool_t adapt_service_times;
  double alpha;
  long int ants;  //!< number of ants for ACO; set to number of customers if 0
  int ants_dynamic;  //!< if true, set ants to the # of customers
  cfg_bool_t best_moves;
  double cost_truck;
  double cost_worker;
  double cost_distance;
  cfg_bool_t deterministic;
  cfg_bool_t do_ls;
  int format;
  double initial_pheromone;
  double lambda;
  long int max_failed_attempts;
  long int max_iterations;  //!< For metaheuristics; 0 for infinite.
  long int max_move;
  long int max_optimize;
  long int max_swap;
  long int max_workers;  //!< Maximum number of workers allowed on a truck.
  int metaheuristic;
  double min_pheromone;
  double mu;
  cfg_bool_t parallel;
  long int rcl_size;  //!< Size of the restricted candidate list (GRASP).
  double rho;  //!< Pheromone persistence.
  long int runtime;  //!< Max. running time per instance [s]. 0 for infinite.
  long int seed;
  double service_rate;
  char* sol_details_filename;
  int start_heuristic;
  char* stats_filename;
  long int tabutime;  //!< Affects the size of the tabu list/ tabu time.
  double truck_velocity;
  cfg_bool_t use_weights;  //!< Use weighted roulette wheel for GRASP.
  long int verbosity;
};

int config_is_valid(Config* cfg);
void config_set_metaheuristic(int*, const char*);
void config_set_output_format(int*, const char*);
void config_set_start_heuristic(int*, const char*);
void fprint_config_summary(FILE* stream, Config* cfg);
void free_config(Config*);
Config *get_config(char* fname);
void print_config(Config* cfg);

#endif
