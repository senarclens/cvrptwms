/** \file
 *
 * \author Gerald Senarclens de Grancy <oss@senarclens.eu>
 * \copyright GNU General Public License version 3
 *
 */

#include <string>
#include <iostream>
#include <unistd.h>
#include <limits.h>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;


// Return the path of the executable.
// Platform dependent solution currently only implemented for Linux.
std::string get_application_path() {
  char buf[PATH_MAX + 1];
  if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) == -1)
    throw std::string("readlink() failed");
  std::string str(buf);
  return str.substr(0, str.rfind('/'));
}


// Return the path to the configuration file.
// Only for the testing facilities.
std::string get_config_path(std::string config_file) {
  fs::path p(get_application_path());
  fs::path filepath(p.parent_path() / "tests" / config_file);
  if (!fs::exists(filepath)) {
    filepath = p.parent_path().parent_path() / "tests" / config_file;
  }
  return filepath.string();
}


std::string get_instance_path(std::string test_instance) {
  fs::path p(get_application_path());
  fs::path filepath(p.parent_path() / "data" / test_instance);
  if (!fs::exists(filepath)) {
    filepath = p.parent_path().parent_path() / "data" / test_instance;
  }
  return filepath.string();
}
