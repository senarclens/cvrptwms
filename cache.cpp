/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <cmath>
#include <iostream>
#include <limits>

extern "C" {
  #include "solution.h"
}

#include "cache.hpp"


Cache::Cache(const Problem& pb) : m_cfg(*(pb.cfg))
{
  m_factor = std::numeric_limits<unsigned long int>::max() / pb.num_nodes;
}



/**
 * Add the solution to the cache and set its counter to 1.
 *
 * This method is meant to be called only if the solution is not in the cache
 * yet. If it was in the cache before, its counter is simply overwritten by 1.
 */
void Cache::add(const Solution& s)
{
  m_cache[hash(s)] = 1;
}


/**
 * Return true if the solution is already in the cache.
 *
 * As a side effect, the number of encounters of this solution is incremented.
 */
unsigned long int Cache::contains(const Solution& s)
{
  unsigned long int hash_value = hash(s);
  auto pair = m_cache.find(hash_value);
  if (pair != m_cache.end()) {
    pair->second++;
    return pair->second;
  }
  return 0;
}


/**
 * Return the number of unique elements in the cache.
 */
long unsigned int Cache::size() const
{
  return m_cache.size();
}


/**
 * Return simple hash of solution struct.
 *
 * This hash is simply a rounded integer representation of the solution value.
 * This function assumes that the program is executed on a 64 bit processor =>
 * the maximum value of long unsigned int is 18,446,744,073,709,551,615.
 */
unsigned long int Cache::hash(const Solution& s) const
{
//   typedef unsigned long int uli;
  // TODO
//   long double cost(calc_cost(&m_cfg, s.trucks, s.workers_cache, s.dist_cache));
//   std::cout << "trucks: " << s.trucks << "\n";  // TODO: remove
//   std::cout << "workers_cache: " << s.workers_cache << "\n";  // TODO: remove
//   std::cout << "dist_cache: " << s.dist_cache << "\n";  // TODO: remove
//   std::cout << "cost: " << cost << "\n";  // TODO: remove
//   uli multiple(std::numeric_limits<uli>::max() / uli(ceill(s.cost_cache)) / 10);
//   long double max_div_10(std::numeric_limits<long double>::max() / 10.0);
//   while (cost < max_div_10) {
//     cost *= 10.0L;
//   }
//   unsigned long int hash_value(static_cast<unsigned long int>(s.cost_cache * multiple));
  long double cost(s.cost_cache);
  unsigned long int hash_value(static_cast<unsigned long int>(cost * m_factor));
//   std::cout << hash_value << "\n";
//   std::cout << "cost: " << cost << "\n";
//   std::cout << "fact: " << m_factor << "\n";
//   std::cout << "hash: " << hash_value << "\n";
  return hash_value;
}


/**
 * Return the number of solutions querying the cache.
 *
 * This number is the total number of elements where elements that were
 * queried more than once are counted multiple times.
 */
unsigned long int Cache::queries() const
{
  unsigned long int num_elements(0);
  for (const std::map<unsigned long int, unsigned long int>::value_type& elem: m_cache) {
    num_elements += elem.second;
  }
  return num_elements;
}


/**
 * Textual representation (some stats) of cache.
 *
 * It outputs how many "equal" solutions occurred and how often they were found.
 * It also adds the total number of solutions in the cache and the percentage
 * of these solutions having been obtained multiple times.
 */
std::ostream& operator<<(std::ostream& os, const Cache& c)
{
  if (c.m_cfg.verbosity >= BASIC_DEBUG) {
    os << "Cache statistics: \n";
    os << c.size() << " elements\n";
    os << c.queries() << " queries\n";
    os << 100.0L * (c.queries() - c.m_cache.size()) / c.queries() << "% hits\n";
  }
  return os;
}

