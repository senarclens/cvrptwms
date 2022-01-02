#include <gtest/gtest.h>
#include <stdlib.h>

#include "common.hpp"

extern "C" {
  #include "../common.h"
  #include "../config.h"
  #include "../node.h"
  #include "../problemreader.h"
  #include "../route.h"
  #include "../solution.h"
  #include "../vrptwms.h"
}

#include "../cache.hpp"

const std::string test_instance("R101_50.txt");
const std::string config_file("test_cache.conf");


// A new one of these is created for each test
class TestCache : public testing::Test {
public:
  Problem* pb;

  virtual void SetUp()
  {
    std::string config_path = get_config_path(config_file);
    Config* cfg = get_config((char *) config_path.c_str());
    std::string instance_path = get_instance_path(test_instance);
    this->pb = get_problem((char *) instance_path.c_str(), cfg);
  }

  virtual void TearDown()
  {
    Config* cfg = pb->cfg;
    free_problem(this->pb);
    free(cfg);
  }
};

TEST_F(TestCache, test_add_one) {
  Cache cache(*pb);
  solve_solomon(pb->sol, (int) pb->cfg->max_workers, pb->sol->num_unrouted);
  calc_costs(pb->sol, pb->cfg);
  cache.add(*(pb->sol));
  ASSERT_TRUE(cache.contains(*(pb->sol)));  // solution is in cache
  ASSERT_TRUE(cache.contains(*(pb->sol)));  // solution is in cache
  ASSERT_EQ(3, cache.queries());  // total of three queries
  ASSERT_EQ(1, cache.size());  // only one unique solution
}

TEST_F(TestCache, test_add_three) {
  Cache cache(*pb);
  Solution* sol1 = new_solution(pb);
  solve_solomon(sol1, (int) pb->cfg->max_workers, sol1->num_unrouted);
  Solution* sol2 = clone_solution(sol1);
  Solution* sol3 = clone_solution(sol1);
  calc_costs(sol1, pb->cfg);
  calc_costs(sol2, pb->cfg);
  calc_costs(sol3, pb->cfg);
  sol2->cost_cache += 1;
  sol3->cost_cache -= 1;
  cache.add(*sol1);
  cache.add(*sol2);
  cache.add(*sol3);
  ASSERT_TRUE(cache.contains(*sol1));  // solution is in cache
  ASSERT_TRUE(cache.contains(*sol2));  // solution is in cache
  ASSERT_TRUE(cache.contains(*sol3));  // solution is in cache
  sol3->cost_cache -= 1;
  ASSERT_FALSE(cache.contains(*sol3));  // modified sol3 is not in cache
  cache.add(*sol3);
  ASSERT_EQ(4, cache.size());  // total of three different solutions
  ASSERT_EQ(7, cache.queries());  // 4 solutions + 3 matching calls to contains
}

TEST_F(TestCache, test_hash) {
  Cache cache(*pb);
  Solution* sol_ptr = new_solution(pb);
  solve_solomon(sol_ptr, (int) pb->cfg->max_workers, sol_ptr->num_unrouted);
  calc_costs(sol_ptr, pb->cfg);
//   fprint_solution(stderr, sol_ptr, pb->cfg, 1);  // TODO: remove
  unsigned long int hash = cache.hash(*sol_ptr);
  sol_ptr->cost_cache += 1;
  ASSERT_NE(hash, cache.hash(*sol_ptr));
  sol_ptr->cost_cache -= 2;
  ASSERT_NE(hash, cache.hash(*sol_ptr));
  sol_ptr->cost_cache += 1;
  ASSERT_EQ(hash, cache.hash(*sol_ptr));

  sol_ptr->cost_cache -= 3.5;
  ASSERT_NE(hash, cache.hash(*sol_ptr));

}
