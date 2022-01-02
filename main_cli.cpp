/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <cstdlib>  // srand48, exit etc.
#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

extern "C" {
  #include <confuse.h>

  #include "common.h"
  #include "config.h"
  #include "problemreader.h"
  #include "solution.h"
  #include "stats.h"
  #include "vrptwms.h"
}

#include "common.hpp"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

//! Return the absolute path to the default configuration file.
static std::string find_default_config_file() {
  fs::path p(CONFIG_DIR);
  fs::path filepath(p / CONFIG_FILENAME);
  return filepath.string();
}


int main (int argc, char** argv) {
  try {
    int first = 1;  // TODO: refactor to use proper c++ vector instead if ResultList
    Resultlist* results = (Resultlist*) NULL;
    Resultlist* tail = (Resultlist*) NULL;
    Config* cfg = get_config((char*) find_default_config_file().c_str());

    po::options_description visible("Usage: " + program_name +  " [options] file... ");
    po::options_description cmdline_options;
    po::options_description hidden("Hidden options");
    visible.add_options()
      ("ants", po::value<long int>()->default_value(cfg->ants),
       "number of ants (0 for automatic)")
      ("deterministic,d", "Use deterministic algorithm (for debugging)")
      ("help,h", "Display this help message")
      ("metaheuristic,m",
      po::value<std::string>()->default_value(METAHEURISTICS[cfg->metaheuristic]),
      "use the given metaheuristic")
      // parallel: repetitive output is suppressed and format=csv is implied
      ("parallel", "optimize output for being run in parallel")
      ("rho", po::value<double>()->default_value(cfg->rho),
       "ACO: pheromone persistence (1 - evaporation)")
      ("runtime,r", po::value<long int>()->default_value(cfg->runtime),
      "Runtime per instance (in seconds)\nset to 0 to disable this limit")
      ("seed", po::value<long int>()->default_value(cfg->seed),
      "Select the seed for the pseudo random number generator (for debugging)")
      ("verbosity,v", po::value<long int>()->default_value(cfg->verbosity),
      "Set the verbosity level")
      ("version", "Display the version number");
    hidden.add_options()
      ("input-files", po::value<std::vector<std::string>>(), "Input files")
    ;
    cmdline_options.add(visible).add(hidden);
    po::positional_options_description p;
    p.add("input-files", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    po::notify(vm);

    config_set_metaheuristic(&cfg->metaheuristic, vm["metaheuristic"].as<std::string>().c_str());
    if (vm.count("deterministic")) {
      cfg->deterministic = (cfg_bool_t) 1;
      cfg->metaheuristic = NO_METAHEURISTIC;
    }

    if (vm.count("parallel")) {
      cfg->format = CSV;
      cfg->parallel = (cfg_bool_t) 1;
    }

    if (vm.count("help")) {
      std::cout << visible;
      return 0;
    }

    if (vm.count("version")) {
      std::cout << PROGRAM_NAME << " " << __DATE__ << " " << __TIME__ << std::endl;
      return 0;
    }

    cfg->ants = vm["ants"].as<long int>();
    cfg->ants_dynamic = !cfg->ants;  // dynamic only if ants is set to 0
    cfg->rho = vm["rho"].as<double>();
    cfg->runtime = vm["runtime"].as<long int>();
    cfg->seed = vm["seed"].as<long int>();
    srand48(cfg->seed);  // initialize randomizer
    cfg->verbosity = vm["verbosity"].as<long int>();

    if (!cfg->parallel) {
      fprint_config_summary(stdout, cfg);
    }

    if(vm.count("input-files")){
      std::vector<std::string> files = vm["input-files"].as<std::vector<std::string>>();
      for (auto file : files) {
        Problem* pb = get_problem(file.c_str(), cfg);
        if (!pb) {
          continue;
        }
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
        free_problem(pb);
      }
      print_results(results, cfg);
    } else {
      fprintf(stderr, "No input files given.\n");
    }
    free_results(results);
    free_config(cfg);
    exit(EXIT_SUCCESS);
  } catch(std::exception& e) {
    std::cerr << "Unhandled Exception caught in main:\n"
    << e.what() << ", application will now exit" << std::endl;
    exit(EXIT_FAILURE);
  }
}
