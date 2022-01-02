/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <confuse.h>

#include "common.h"
#include "config.h"
#include "problemreader.h"
#include "solution.h"
#include "stats.h"
#include "vrptwms.h"


///////////////////////////////////////////////////////////////////////////////
// File Scope (Static) Functions                                             //
///////////////////////////////////////////////////////////////////////////////

static void print_usage(char *progname);

//! Print a help message and exit the program.
static void print_help(char *progname, Config *cfg) {
  char *indent = "                         ";
  char *lo = "      "; // indent for long options
  print_usage(progname);
  printf("\nThe values below presented as \"currently set to\" are evaluated");
  printf("\nin the following order: defaults which are overridden by the\n");
  printf("configuration file which is overridden by all options up to the\n");
  printf("the option issuing this help message.\n\n");
  printf("%s--alpha=%%f         ", lo);
  printf("parameter alpha for Solomon's I1 heuristic\n");
  printf("%salpha needs to be in the intervall [0.0, 1.0]\n", indent);
  printf("%scurrently set to %.1f\n", indent, cfg->alpha);
  printf("%s--ants=%%d          ", lo);
  printf("number of ants; currently set to %ld\n", cfg->ants);
  printf("  -c  --construct=%%s     ");
  printf("select route construction heuristic\n");
  printf("%s'%s' for Solomon I1\n", indent, START_HEURISTICS[SOLOMON]);
  printf("%s'%s' for parallel construction\n", indent,
         START_HEURISTICS[PARALLEL]);
  printf("%scurrently set to '%s'\n", indent,
         START_HEURISTICS[cfg->start_heuristic]);
  printf("%s--%-17s", lo, "format=%%s");
  printf("print results as\n%s'%s' for human readable\n%s'%s' for csv\n",
         indent, OUTPUT_FORMATS[HUMAN], indent, OUTPUT_FORMATS[CSV]);
  printf("%scurrently set to '%s'\n", indent, OUTPUT_FORMATS[cfg->format]);
  printf("  -d  --deterministic    ");
  printf("use deterministic solution construction\n");
  printf("%s--%-17s", lo, "grasp-rcl-size=%d ");
  printf("size of the restricted candidate list (GRASP)\n");
  printf("%scurrently set to %ld\n", indent, cfg->rcl_size);
  printf("%s--%-17s", lo, "grasp-use-weights=%d ");
  printf("enable (1)/ disable (0) weights for selecting from RCL (GRASP)\n");
  printf("%scurrently set to %d\n", indent, cfg->use_weights);
  printf("  -h  --help             ");
  printf("display this help and exit\n");
  printf("%s--iterations=%%d     ", lo);
  printf("maximum number of iterations before the algorithm stops\n");
  printf("%swill be rounded up to the closest multiple of 'ants'\n", indent);
  printf("%sset to %d to disable this limit\n", indent, UNLIMITED);
  printf("%scurrently set to %ld\n", indent, cfg->max_iterations);
  printf("%s--ls=%%d            enable (1)/ disable (0) local search\n", lo);
  printf("%scurrently set to %d\n", indent, cfg->do_ls);
  printf("  -m  --metaheuristic=%%s ");
  printf("use the given metaheuristic\n%s'%s' to disable metaheuristics\n",
         indent, METAHEURISTICS[NO_METAHEURISTIC]);
  printf("%s'%s' for ant colony optimization\n", indent, METAHEURISTICS[ACO]);
  printf("%s'%s' for ACO using cache\n", indent, METAHEURISTICS[CACHED_ACO]);
  printf("%s'%s' for greedy randomized adaptive search procedure\n", indent,
         METAHEURISTICS[GRASP]);
  printf("%s'%s' for tabu search\n", indent, METAHEURISTICS[TS]);
  printf("%scurrently set to '%s'\n", indent,
         METAHEURISTICS[cfg->metaheuristic]);
  printf("%s--parallel         ", lo);
  printf("adjust output format if the program is run with GNU parallel\n");
  printf("%srepetitive output is suppressed and format=csv is implied\n",
         indent);
  printf("%s--print-config     ", lo);
  printf("print the used configuration (only use as last option)\n");
  printf("  -r  --runtime=%%d       ");
  printf("runtime per instance (in seconds)\n");
  printf("%sset to %d to disable this limit\n", indent, UNLIMITED);
  printf("%scurrently set to %ld\n", indent, cfg->runtime);
  printf("%s--seed=%%ld         ", lo);
  printf("select the seed for the pseudo random number generator\n");
  printf("  -v  --verbose          ");
  printf("increase the configuration's verbosity level by one\n");
  printf("%scan be used multiple times\n", indent);
  printf("%scurrently set to %ld\n", indent, cfg->verbosity);
  printf("%s--vrptw            solve a regular VRPTW\n", lo);
  printf("%s(only 1 worker per vehicle, no service time adaption)\n", indent);
  exit(0);
}


//! Print a short usage message.
static void print_usage(char *progname) {
  printf("Usage: %s [OPTIONS] infile1 [infile2] [...]\n", progname);
}


///////////////////////////////////////////////////////////////////////////////
// main                                                                      //
///////////////////////////////////////////////////////////////////////////////

//! Entry point for the command-line interface.
int main(int argc, char *argv[]) {
  int c = 0;
  Config* cfg = get_config(CONFIG);
  int first = 1;
  Resultlist* results = (Resultlist*) NULL;
  Resultlist* tail = (Resultlist*) NULL;
  while (1) {
    static struct option long_options[] = {  // highest used id: 1010
    {"alpha",             required_argument, 0, 1000},
    {"ants",              required_argument, 0, 1005},
    {"construct",         required_argument, 0,  'c'},
    {"deterministic",     no_argument,       0,  'd'},
    {"format",            required_argument, 0, 1003},
    {"grasp-rcl-size",    required_argument, 0, 1009},
    {"grasp-use-weights", required_argument, 0, 1010},
    {"help",              no_argument,       0,  'h'},
    {"iterations",        required_argument, 0, 1006},
    {"ls",                required_argument, 0, 1004},
    {"metaheuristic",     required_argument, 0,  'm'},
    {"parallel",          no_argument,       0, 1008},
    {"print-config",      no_argument,       0, 1001},
    {"runtime",           required_argument, 0,  'r'},
    {"seed",              required_argument, 0, 1002},
    {"verbose",           no_argument,       0,  'v'},
    {"vrptw",             no_argument,       0, 1007},
    {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;
    c = getopt_long (argc, argv, "c:dhm:r:v",
                     long_options, &option_index);
    /* Detect the end of the options. */
    if (c == -1) break;
    switch (c) {
      case 'h': // help should be the first option
      print_help(argv[0], cfg);
      break;
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;
        printf ("long option %s", long_options[option_index].name);
        if (optarg)
          printf (" with arg %s", optarg);
        printf ("\n");
        break;
      case 1000:  // --alpha=
        cfg->alpha = atof(optarg);
        break;
      case 1001:  // print-config
        print_config(cfg);
        break;
      case 1002:  // --seed=
        cfg->seed = atol(optarg);
        break;
      case 1003:  // --format=
        config_set_output_format(&cfg->format, optarg);
        break;
      case 1004:  // --do_ls=
        cfg->do_ls = (cfg_bool_t) atoi(optarg);
        break;
      case 1005:  // --ants=
        cfg->ants = atoi(optarg);
        cfg->ants_dynamic = 0;
        break;
      case 1006:  // --iterations=
        cfg->max_iterations = atol(optarg);
        break;
      case 1007:  // --vrptw
        cfg->adapt_service_times = (cfg_bool_t) 0;
        cfg->max_workers = 1;
        break;
      case 1008:  // --parallel
        cfg->format = CSV;
        cfg->parallel = (cfg_bool_t) 1;
        break;
      case 1009:  // --grasp-rcl-size=
        cfg->rcl_size = atol(optarg);
        break;
      case 1010:  // --grasp-use-weights=
        cfg->use_weights = (cfg_bool_t) atoi(optarg);
        break;
      case 'c':
        config_set_start_heuristic(&cfg->start_heuristic, optarg);
        break;
      case 'm':  // needs to be set before 'd'
        config_set_metaheuristic(&cfg->metaheuristic, optarg);
        break;
      case 'd':
        cfg->deterministic = (cfg_bool_t) 1;
        cfg->metaheuristic = NO_METAHEURISTIC;
        break;
      case 'v':
        cfg->verbosity++;
        break;
      case 'r':
        cfg->runtime = atol(optarg);
        break;
      case '?':
        // getopt_long already printed an error message.
        break;
      default:
        abort();
    }
  }
  if (!config_is_valid(cfg)) {
    fprintf(stderr, "invalid configuration, exiting\n");
    exit(EXIT_FAILURE);
  }
  srand48(cfg->seed);

  if (!cfg->parallel)
    fprint_config_summary(stdout, cfg);

  if (optind == argc) {
    fprintf(stderr, "No input files given.\n");
    print_usage(argv[0]);
    printf("\n");
    exit(EXIT_FAILURE);
  }
  // process any remaining command line arguments (not options)
  while (optind < argc) {
    if (cfg->verbosity >= BASIC_VERBOSITY) {
      printf ("====================\n");
      printf ("processing \"%s\"...\n", argv[optind]);
    }
    Problem *pb = get_problem(argv[optind++], cfg);
    if (!pb)
      continue;
    solve(pb, (int) pb->cfg->max_workers, pb->sol->num_unrouted);
    assert_feasibility(pb->sol);
    if (cfg->verbosity >= BASIC_DEBUG)
      fprint_solution(stdout, pb->sol, cfg, (int) cfg->verbosity);
    save_solution_details(pb->sol, cfg);
    if (first) {
      first = 0;
      results = add_result(pb);
      tail = results;
    } else {
      tail->next = add_result(pb);
      tail = tail->next;
    }
    write_stats(pb->stats, cfg->stats_filename);
    free_problem(pb);
  }
  print_results(results, cfg);
  free_results(results);
  free_config(cfg);
  exit(EXIT_SUCCESS);
}
