/*
 * Test the basic running configurations to see if they don't crash.
 * This test should be run on each change.
 */

#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.hpp"

extern "C" {
  #include "../ant_colony_optimization.h"
  #include "../common.h"
  #include "../config.h"
  #include "../grasp.h"
  #include "../node.h"
  #include "../problemreader.h"
  #include "../route.h"
  #include "../solution.h"
  #include "../tabu_search.h"
  #include "../vrptwms.h"
}

const std::string test_instance("R101.txt");
const std::string config_file("testing.conf");


// A new one of these is created for each test
class QuickTest : public testing::Test {
public:
  Problem* pb;

  virtual void SetUp()
  {
    std::string config_path = get_config_path(config_file);
    Config* cfg = get_config((char *) config_path.c_str());
    std::string instance_path = get_instance_path(test_instance);
    cfg->seed = 0;
    cfg->runtime = 0;
    cfg->max_iterations = 70;
    cfg->do_ls = (cfg_bool_t) 0;
    this->pb = get_problem((char *) instance_path.c_str(), cfg);
    if (!pb) {
      std::cout << "ERROR: cannot open data file ";
      std::cout << "(" << instance_path << ") for testing\n";
      exit(EXIT_FAILURE);
    }
  }

  virtual void TearDown()
  {
    Config* cfg = pb->cfg;
    free_problem(this->pb);
    free(cfg);
  }
};

TEST_F(QuickTest, deterministic_solomon) {
  pb->cfg->metaheuristic = NO_METAHEURISTIC;
  pb->cfg->deterministic = (cfg_bool_t) 1;
  solve(pb, (int) pb->cfg->max_workers, pb->sol->num_unrouted);
  assert_feasibility(pb->sol);
  ASSERT_EQ(21, pb->sol->trucks);
  int workers(0);
  for (int i = 0; i < pb->sol->trucks; i++) {
    workers += pb->sol->routes[i]->workers;
  }
  ASSERT_EQ(55, workers);  // without local search!
}

TEST_F(QuickTest, stochastic_solomon) {
  pb->cfg->metaheuristic = NO_METAHEURISTIC;
  solve(pb, (int) pb->cfg->max_workers, pb->sol->num_unrouted);
  assert_feasibility(pb->sol);
}

TEST_F(QuickTest, run_aco_parallel) {
  pb->cfg->metaheuristic = ACO;
  pb->cfg->ants = 50;
  pb->cfg->start_heuristic = PARALLEL;
  solve(pb, (int) pb->cfg->max_workers, pb->sol->num_unrouted);
  assert_feasibility(pb->sol);
}

TEST_F(QuickTest, run_aco) {
  pb->cfg->metaheuristic = ACO;
  pb->cfg->ants = 50;
  pb->cfg->start_heuristic = SOLOMON;
  solve(pb, (int) pb->cfg->max_workers, pb->sol->num_unrouted);
  assert_feasibility(pb->sol);
}

TEST_F(QuickTest, run_aco_ls) {
  pb->cfg->metaheuristic = ACO;
  pb->cfg->ants = 50;
  pb->cfg->do_ls = (cfg_bool_t) 1;
  pb->cfg->start_heuristic = SOLOMON;
  solve(pb, (int) pb->cfg->max_workers, pb->sol->num_unrouted);
  assert_feasibility(pb->sol);
}

TEST_F(QuickTest, run_ts) {
  pb->tl->active = 1;  // required as the problem was initialized w/ ACO
  pb->cfg->metaheuristic = TS;
  pb->cfg->start_heuristic = SOLOMON;
  solve(pb, (int) pb->cfg->max_workers, pb->sol->num_unrouted);
  assert_feasibility(pb->sol);
}

TEST_F(QuickTest, run_grasp_rcl) {
  pb->cfg->metaheuristic = GRASP;
  pb->cfg->start_heuristic = SOLOMON;
  pb->cfg->use_weights = (cfg_bool_t) 0;
  pb->cfg->rcl_size = 20;
  solve(pb, (int) pb->cfg->max_workers, pb->sol->num_unrouted);
  assert_feasibility(pb->sol);
}

TEST_F(QuickTest, run_grasp_weighted) {
  pb->cfg->metaheuristic = GRASP;
  pb->cfg->start_heuristic = SOLOMON;
  pb->cfg->use_weights = (cfg_bool_t) 1;
  pb->cfg->rcl_size = 0;
  solve(pb, (int) pb->cfg->max_workers, pb->sol->num_unrouted);
  assert_feasibility(pb->sol);
}

TEST_F(QuickTest, run_vns) {
  pb->cfg->metaheuristic = VNS;
  pb->cfg->start_heuristic = SOLOMON;
  solve(pb, (int) pb->cfg->max_workers, pb->sol->num_unrouted);
    assert_feasibility(pb->sol);
}

