/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <confuse.h>

#include "wrappers.h"
#include "config.h"


const char* METAHEURISTICS[] = {
  [NO_METAHEURISTIC] = "none",
  [ACO] = "aco",
  [CACHED_ACO] = "cached_aco",
  [CACHED_GRASP] = "cached_grasp",
  [GACO] = "gaco",
  [GRASP] = "grasp",
  [VNS] = "vns",
  [TS] = "ts",
};
const char* START_HEURISTICS[] = {
  [SOLOMON] = "solomon",
  [SOLOMON_MR] = "solomon-mr",
  [PARALLEL] = "parallel",
};
const char* OUTPUT_FORMATS[] = {
  [HUMAN] = "human",
  [CSV] = "csv",
};

///////////////////////////////////////////////////////////////////////////////
// File Scope (Static) Functions                                             //
///////////////////////////////////////////////////////////////////////////////
static void config_fprintf_enum(const char* arg, int first, int last,
                                const char* array[], char* label);
static void config_set_enum(const char* arg, int first, int last,
                            const char* array[], char* label, int* val);
static char* get_output_format(Config* cfg);


//! Print error message explaining which arguments an option can take.
//! \param arg config value or option argument given by the user
//! \param first enum's first index
//! \param last enum's last index
//! \param array string array containing possible values
//! \param label name of the config value to be set
static void config_fprintf_enum(const char* arg, int first, int last,
                                const char* array[], char* label) {
  fprintf(stderr, "ERROR: %s '%s' not recognized\n", label, arg);
  fprintf(stderr, "can be any of ");
  for (int i = first; i < last; ++i)
    fprintf(stderr, "'%s', ", array[i]);
  fprintf(stderr, "'%s'\n", array[last]);
}


//! Set default values to passed config.
void config_set_default_values(Config* cfg) {
  cfg->adapt_service_times = cfg_true;
  cfg->alpha = 1.0;
  cfg->ants = 0;
  cfg->best_moves = cfg_true;
  cfg->cost_truck = 1.0;
  cfg->cost_worker = 0.1;
  cfg->cost_distance = 0.0001;
  cfg->deterministic = cfg_false;
  cfg->do_ls = cfg_true;
  config_set_output_format(&cfg->format, "human");
  cfg->initial_pheromone = 1.0;
  cfg->lambda = 2.0;
  cfg->max_failed_attempts = 500L;
  cfg->max_iterations = 0L;
  cfg->max_move = 2L;
  cfg->max_optimize = 3L;
  cfg->max_swap = 1L;
  cfg->max_workers = 3L;
  config_set_metaheuristic(&cfg->metaheuristic, "aco");
  cfg->min_pheromone = 0.0000000000001;
  cfg->mu = 1.0;
  cfg->parallel = cfg_false;
  cfg->rcl_size = 2;
  cfg->rho = 0.985;
  cfg->runtime = 10L;
  cfg->service_rate = 2.0;
  cfg->sol_details_filename = s_malloc(sizeof(char) * 12);
  strcpy(cfg->sol_details_filename, "details.txt");
  config_set_start_heuristic(&cfg->start_heuristic, "solomon");
  cfg->stats_filename = s_malloc(sizeof(char) * 10);
  strcpy(cfg->stats_filename, "stats.txt");
  cfg->tabutime = 50;
  cfg->truck_velocity = 1.0;
  cfg->use_weights = cfg_true;
  cfg->verbosity = 0L;
}


//! Set config value "val" based on the string "arg".
//! \param arg config value or option argument given by the user
//! \param first enum's first index
//! \param last enum's last index
//! \param array string array containing possible values
//! \param label name of the config value to be set
//! \param val pointer to the config value that needs to be set
static void config_set_enum(const char* arg, int first, int last,
                            const char* array[], char* label, int* val) {
  if (strcmp(arg, NOT_SET) == 0) {
    fprintf(stderr, "WARNING: %s not configured\n", label);
    fprintf(stderr, "defaulting to '%s'\n",
            array[first]);
    *val = first;
    return;
  }
  for (int i = first; i <= last; ++i) {
    if (strcmp(arg, array[i]) == 0) {
      *val = i;
      return;
    }
  }
  config_fprintf_enum(arg, first, last, array, label);
  exit(EXIT_FAILURE);
}


//! Print the parameters of the local search.
static void fprint_local_search(FILE* stream, Config* cfg) {
  if (cfg->do_ls) {
    fprintf(stream, "local search ");
    if (cfg->best_moves)
      fprintf(stream, "(only best moves; max_move: %ld, max_swap: %ld)\n",
             cfg->max_move, cfg->max_swap);
    else
      fprintf(stream, "(first improving moves; max_move: %ld, max_swap: %ld)\n",
             cfg->max_move, cfg->max_swap);
  } else {
    fprintf(stream, "no local search\n");
  }
}


//! Print the name and parameters of the used metaheuristic.
static void fprint_metaheuristic(FILE* stream, Config* cfg) {
  switch (cfg->metaheuristic) {
    case CACHED_ACO:
      fprintf(stream, "cached ");
    case ACO:
      if (cfg->ants) {
        fprintf(stream, "ant colony optimization (ants: %ld, rho: %.3f, ",
                cfg->ants, cfg->rho);
      } else {
        fprintf(stream, "ant colony optimization (ants: dynamic, rho: %.3f, ",
                cfg->rho);
      }
      fprintf(stream, "min. ph.:%.3f)\n", cfg->min_pheromone);
      break;
    case CACHED_GRASP:
      fprintf(stream, "cached ");
    case GRASP:
      fprintf(stream, "grasp (rcl-size: %ld, use-weights: ", cfg->rcl_size);
      if (cfg->use_weights)
        fprintf(stream, "yes");
      else
        fprintf(stream, "no");
      fprintf(stream, ")\n");
      break;
    case TS:
      fprintf(stream, "tabu search\n");
      break;
    default:
      fprintf(stream, "%s\n", METAHEURISTICS[cfg->metaheuristic]);
      break;
  }
}


//! Print the type and parameters of the route construction heuristic.
static void fprint_start_heuristic(FILE* stream, Config* cfg) {
  if (cfg->start_heuristic == SOLOMON) {
    if (cfg->deterministic) {
      fprintf(stream, "deterministic ");
    } else {
      fprintf(stream, "stochastic ");
    }
  }
  fprintf(stream, "%s ", START_HEURISTICS[cfg->start_heuristic]);
  fprintf(stream, "(alpha: %.2f, lambda: %.2f and mu: %.2f)\n",
         cfg->alpha, cfg->lambda, cfg->mu);
}


//! Return the output format as string.
char* get_output_format(Config* cfg) {
  switch (cfg->format) {
    case HUMAN:
      return "human readable";
    case CSV:
      return "csv";
  }
  return "undefined";
}


///////////////////////////////////////////////////////////////////////////////
// Public Functions                                                          //
///////////////////////////////////////////////////////////////////////////////


//! Return 1 if the config is valid, otherwise 0.
int config_is_valid(Config* cfg) {
  int valid = 1;
  if (cfg->runtime < 0) {
    fprintf(stderr, "ERROR: the runtime has to be >= 0 (0 for infinite)\n");
    valid = 0;
  }
  if (cfg->max_iterations < 0) {
    fprintf(stderr, "ERROR: max_iterations has to be >= 0 (0 for infinite)\n");
    valid = 0;
  }
  if (!cfg->runtime && !cfg->max_iterations) {
    fprintf(stderr, "ERROR: iterations or runtime must be finite (> 0)\n");
    valid = 0;
  }
  if (cfg->max_move < 0) {
    fprintf(stderr, "ERROR: max_move has to be >= 0)\n");
    valid = 0;
  }
  if (cfg->max_swap < 0) {
    fprintf(stderr, "ERROR: max_swap has to be >= 0)\n");
    valid = 0;
  }
  return valid;
}


//! Set the metaheuristic based on the given string.
void config_set_metaheuristic(int* val, const char* arg) {
  config_set_enum(arg, FIRST_METAHEURISTIC, LAST_METAHEURISTIC,
                  METAHEURISTICS, "metaheuristic", val);
}


//! Set the output format based on the given string.
void config_set_output_format(int* val, const char* arg) {
  config_set_enum(arg, FIRST_OUTPUTFORMAT, LAST_OUTPUTFORMAT,
                  OUTPUT_FORMATS, "output format", val);
}


//! Set the start heuristic based on the given string.
void config_set_start_heuristic(int* val, const char* arg) {
  config_set_enum(arg, FIRST_STARTHEURISTIC, LAST_STARTHEURISTIC,
                  START_HEURISTICS, "start heuristic", val);
}


//! Free the given configuration.
void free_config(Config* cfg) {
  free(cfg->stats_filename);
  free(cfg->sol_details_filename);
  free(cfg);
}


//! Print a summary of the actual key configuration parameters.
//! This configuration includes all commandline arguments processed
//! when this function is called.
void fprint_config_summary(FILE* stream, Config* cfg) {
  if (stream == stdout)
    fprintf(stream, "output format: %s; ", get_output_format(cfg));
  fprintf(stream, "seed: %ld\n", cfg->seed);
  fprint_metaheuristic(stream, cfg);
  fprint_start_heuristic(stream, cfg);
  fprint_local_search(stream, cfg);
  if (cfg->metaheuristic) {
    if (cfg->runtime)
      fprintf(stream, "runtime: %ld sec/ inst\n", cfg->runtime);
    else
      fprintf(stream, "runtime not limited; max. %ld iterations\n",
              cfg->max_iterations);
  }
  if (stream == stdout)
    fprintf(stream, "\n");
}


//! Parse the configuration file and initialize the config struct.
//! \return pointer to the configuration
Config* get_config(char *fname) {
  Config* cfg = (Config*) s_malloc(sizeof(Config));
  char* sol_details_filename = (char*) NULL;
  char* stats_filename = (char*) NULL;
  cfg_opt_t opts[] = {
    CFG_SIMPLE_BOOL("adapt_service_times", &cfg->adapt_service_times),
    CFG_SIMPLE_FLOAT("alpha", &cfg->alpha),
    CFG_SIMPLE_INT("ants", &cfg->ants),
    CFG_SIMPLE_BOOL("best_moves", &cfg->best_moves),
    CFG_SIMPLE_FLOAT("cost_truck", &cfg->cost_truck),
    CFG_SIMPLE_FLOAT("cost_worker", &cfg->cost_worker),
    CFG_SIMPLE_FLOAT("cost_distance", &cfg->cost_distance),
    CFG_SIMPLE_BOOL("deterministic", &cfg->deterministic),
    CFG_SIMPLE_BOOL("do_ls", &cfg->do_ls),
    CFG_STR("format", NOT_SET, CFGF_NONE),
    CFG_SIMPLE_FLOAT("initial_pheromone", &cfg->initial_pheromone),
    CFG_SIMPLE_FLOAT("lambda", &cfg->lambda),
    CFG_SIMPLE_INT("max_failed_attempts", &cfg->max_failed_attempts),
    CFG_SIMPLE_INT("max_iterations", &cfg->max_iterations),
    CFG_SIMPLE_INT("max_move", &cfg->max_move),
    CFG_SIMPLE_INT("max_optimize", &cfg->max_optimize),
    CFG_SIMPLE_INT("max_swap", &cfg->max_swap),
    CFG_SIMPLE_INT("max_workers", &cfg->max_workers),
    CFG_STR("metaheuristic", NOT_SET, CFGF_NONE),
    CFG_SIMPLE_FLOAT("min_pheromone", &cfg->min_pheromone),
    CFG_SIMPLE_FLOAT("mu", &cfg->mu),
    CFG_SIMPLE_BOOL("parallel", &cfg->parallel),
    CFG_SIMPLE_INT("rcl_size", &cfg->rcl_size),
    CFG_SIMPLE_FLOAT("rho", &cfg->rho),
    CFG_SIMPLE_INT("runtime", &cfg->runtime),
    CFG_SIMPLE_FLOAT("service_rate", &cfg->service_rate),
    CFG_SIMPLE_STR("sol_details_filename", &sol_details_filename),
    CFG_STR("start_heuristic", NOT_SET, CFGF_NONE),
    CFG_SIMPLE_STR("stats_filename", &stats_filename),
    CFG_SIMPLE_INT("tabutime", &cfg->tabutime),
    CFG_SIMPLE_FLOAT("truck_velocity", &cfg->truck_velocity),
    CFG_SIMPLE_BOOL("use_weights", &cfg->use_weights),
    CFG_SIMPLE_INT("verbosity", &cfg->verbosity),
    CFG_END()
  };
  cfg_t *parsed;
  parsed = cfg_init(opts, CFGF_NONE);
  switch (cfg_parse(parsed, fname)) {
    case CFG_FILE_ERROR:
      printf("WARNING: configuration file '%s' could not be read: %s\n",
             fname, strerror(errno));
      printf("continuing with default values...\n\n");
      config_set_default_values(cfg);
      break;
    case CFG_PARSE_ERROR:
      fprintf(stderr, "get_config: error reading config\n");
      exit(EXIT_FAILURE);
      break;
    case CFG_SUCCESS:
      config_set_metaheuristic(&cfg->metaheuristic,
                               cfg_getstr(parsed, "metaheuristic"));
      config_set_output_format(&cfg->format, cfg_getstr(parsed, "format"));
      config_set_start_heuristic(&cfg->start_heuristic,
                                 cfg_getstr(parsed, "start_heuristic"));
      cfg->sol_details_filename = sol_details_filename;
      cfg->stats_filename = stats_filename;
      cfg_free(parsed);
      break;
  }
  cfg->seed = time((time_t*) NULL);
  cfg->ants_dynamic = !cfg->ants;
  return cfg;
}


//! Print a thorough representation of the current configuration.
//! This configuration includes all commandline arguments processed
//! when this function is called.
void print_config(Config *cfg) {
  printf ("\nConfiguration:\n==============\n\n");
  printf("note: the shown config includes all commandline arguments\n");
  switch (cfg->verbosity)
  {
    case MIN_VERBOSITY:
      printf("compact output\n");
      break;
    case BASIC_VERBOSITY:
      printf("print solution details, random seed, ...\n");
    case BASIC_DEBUG:
      printf("print basic debug output\n");
  }
  printf("output format: %s\n", get_output_format(cfg));
  printf("max workers per truck: %ld\n\n", cfg->max_workers);
  printf("metaheuristic: ");
  fprint_metaheuristic(stdout, cfg);
  if (cfg->metaheuristic) {
    printf("runtime: %ld seconds per instance\n", cfg->runtime);
  }
  printf ("\nstart heuristic: ");
  fprint_start_heuristic(stdout, cfg);
  printf ("\ncosts: %.3f/truck, %.3f/worker, %.4f/distance unit\n",
    cfg->cost_truck, cfg->cost_worker, cfg->cost_distance);

  fprint_local_search(stdout, cfg);

  printf ("\nservice times are ");
  if (!cfg->adapt_service_times) printf ("NOT ");
  printf ("adapted; ");
  if (cfg->adapt_service_times) {
  printf ("service_rate: %.3f ", cfg->service_rate);
  printf ("truck_velocity: %.3f\n", cfg->truck_velocity);
  } else {
  printf ("\n");
  }
  printf("==============\n\n");
}


