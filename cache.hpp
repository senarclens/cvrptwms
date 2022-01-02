/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#ifndef CACHE_H
#define CACHE_H

#include <map>

#include <gtest/gtest_prod.h>

extern "C" {
  #include "problemreader.h"
  #include "config.h"
}

#include "common.h"

/**
 * A simple representation of a cache for storing past solutions.
 *
 * The purpose of the cache is to quickly determine if a newly obtained
 * solution has already been obtained in the past. This knowledge allows
 * skipping the expensive local search if it has already been performed.
 *
 * This caches is realized as a C++ mapping of solution hash values to the
 * number of times a particular hash has been encountered.
 *
 * The implementation below is extremely simplistic and mainly serves as a
 * proof of concept - it does not compare solutions for identity, but only
 * uses the solution's objective function value. This could lead to skipping
 * different solutions with identical objective function value. If this poses
 * a problem can be investigated later in case needed, but for now we go with
 * 'simple is beautiful'.
 *
 * Some alternative hashing strategies include
 * - sum(node->alst - node->aest) for all nodes
 * - sum(node->aest) for all nodes
 * - sum(node->alst) for all nodes
 * - length of longest route
 * - length of shortest route
 * - sum of node ids of all routes first non-depot nodes
 * - sum of node ids of all routes last non-depot nodes
 * - hash routes separately, then sum up the route hashes to avoid permutation issues
 * Note that these strategies can also be combined.
 */
class Cache {
public:
  explicit Cache(const Problem& pb);

  void add(const Solution& s);
  long unsigned int contains(const Solution& s);
  unsigned long int size() const;
//   unsigned long int memory();

private:
  unsigned long int hash(const Solution& s) const;
  unsigned long int queries() const;

  std::map<unsigned long int,unsigned long int> m_cache;
  const Config& m_cfg;
  unsigned long int m_factor;

  friend std::ostream& operator<< (std::ostream& os, const Cache& c);

  FRIEND_TEST(TestCache, test_add_one);
  FRIEND_TEST(TestCache, test_add_three);
  FRIEND_TEST(TestCache, test_hash);
};

std::ostream& operator<< (std::ostream& os, const Cache& c);

#endif  // CACHE_H
