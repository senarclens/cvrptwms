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

const std::string test_instance("R101_25.txt");
const std::string config_file("testing.conf");

/* first part of the data file R101_25.txt
 * (from http://web.cba.neu.edu/~msolomon/problems.htm but starting with 0)
R101

VEHICLE
NUMBER     CAPACITY
   8         200

CUSTOMER
CUST NO.   XCOORD.   YCOORD.    DEMAND   READY TIME   DUE DATE   SERVICE TIME

    0          35      35           0       0         230           0
    1          41      49          10     161         171          10
    2          35      17           7      50          60          10
    3          55      45          13     116         126          10
    4          55      20          19     149         159          10
    5          15      30          26      34          44          10
    6          25      30           3      99         109          10
    7          20      50           5      81          91          10
    8          10      43           9      95         105          10
    9          55      60          16      97         107          10
   10          30      60          16     124         134          10
   ...
 */


TEST(TestProblemreader, get_problem) {
  std::string config_path = get_config_path(config_file);
  Config* cfg = get_config((char *) config_path.c_str());
  std::string instance_path = get_instance_path(test_instance);
  Problem* pb = get_problem((char *) instance_path.c_str(), cfg);
  ASSERT_EQ(cfg, pb->cfg);
  ASSERT_EQ(200, pb->capacity);
  ASSERT_EQ(0, pb->num_solutions);
  ASSERT_EQ(26, pb->num_nodes);
  ASSERT_STREQ("R101_25", pb->name);
  ASSERT_TRUE(pb->c_m);
  ASSERT_TRUE(pb->nodes);
  ASSERT_TRUE(pb->pheromone);
  ASSERT_TRUE(pb->sol);
  ASSERT_TRUE(pb->tl);
  free_problem(pb);
  free(cfg);
}
