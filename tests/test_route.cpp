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
}

const std::string test_instance("R101.txt");
const std::string config_file("testing.conf");


// A new one of these is created for each test
class TestRoute : public testing::Test {
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

TEST_F(TestRoute, test_new_route) {
  Node* seed = pb->sol->unrouted;
  remove_unrouted(pb->sol, seed);  // this needs to be done before new_route!!
  Route* r = new_route(pb->sol, seed, (int) pb->cfg->max_workers);
  ASSERT_EQ(3, r->len);
  ASSERT_EQ(seed->demand, r->load);
  ASSERT_EQ(DEPOT, r->nodes->id);  // start w/ depot
  ASSERT_EQ(seed, r->nodes->next);  // seed node is on route
  ASSERT_EQ(DEPOT, r->tail->id);  // end w/ depot
}

