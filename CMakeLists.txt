# VRPTWMS solver
# Copyright (C) 2013  Gerald Senarclens de Grancy <oss@senarclens.eu>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 2.8)

project(cvrptwms)

# Compiler options
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Wextra -Wconversion")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -ggdb3 -fbounds-check -pg -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wconversion")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -std=c++11 -Wall -Wextra -Wconversion -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
# build with cmake .. -DCMAKE_BUILD_TYPE=Release

set(BOOST_MIN_VERSION "1.53.0")
find_package(Boost ${BOOST_MIN_VERSION}
  COMPONENTS filesystem
             #math_tr1
             program_options
             system
             #timer
  REQUIRED)
# include_directories(${INCLUDE_DIRECTORIES} ${Boost_INCLUDE_DIRS})
# include_directories(${INCLUDE_DIRECTORIES} ${PROJECT_INCLUDE_DIR})
# link_directories(${LINK_DIRECTORIES} ${Boost_LIBRARY_DIRS})

set(CLIBNAME ${PROJECT_NAME}_c)  # the library to be linked against
set(CPPLIBNAME ${PROJECT_NAME}_cpp)  # the cpp library to be linked against
set(LIBS ${LIBS}
  boost_filesystem
  boost_program_options
  boost_system
#  boost_timer
)

set(CONFIG_INSTALL_DIR "share/config") # The config file install dir
set(CONFIG_FILENAME "vrptwms.conf")
add_definitions(-DCONFIG_DIR="${CMAKE_INSTALL_PREFIX}/${CONFIG_INSTALL_DIR}")
add_definitions(-DCONFIG_FILENAME="${CONFIG_FILENAME}")
add_definitions(-DCONFIG="${CMAKE_INSTALL_PREFIX}/${CONFIG_INSTALL_DIR}/${CONFIG_FILENAME}")
add_definitions(-DPROGRAM_NAME="${PROJECT_NAME}")
# string(TIMESTAMP VERSION "%Y-%m-%d %H:%M")
# add_definitions(-DVERSION="${VERSION}")

set(OLD_CLI_FILE cli.c)
set(OLD_CLI_EXECUTABLE cvrptwms)
set(CLI_FILE main_cli.cpp)
set(CLI_EXECUTABLE ${PROJECT_NAME}_cli)

set(C_SRCS  # all non-main source files used by the pure C version
  ant_colony_optimization.c
  common.c
  config.c
  grasp.c
  local_search.c
  node.c
  problemreader.c
  route.c
  solution.c
  stats.c
  tabu_search.c
  vns.c
  vrptwms.c
  wrappers.c
)

set(CPP_SRCS  # all non-main c++ source files
  cache.cpp
  cached_aco.cpp
  cached_grasp.cpp
)

# link_directories(${LINK_DIRECTORIES} "/home/gerald/repos/cvrptwms/build")
add_executable(${OLD_CLI_EXECUTABLE} ${C_SRCS} ${OLD_CLI_FILE})
target_link_libraries(${OLD_CLI_EXECUTABLE} confuse m ${CPPLIBNAME})

add_executable(${CLI_EXECUTABLE} ${CLI_FILE})
target_link_libraries(${CLI_EXECUTABLE} ${CLIBNAME} ${CPPLIBNAME} confuse ${LIBS})

add_library(${CLIBNAME} ${C_SRCS})
target_link_libraries(${CLIBNAME} confuse m ${CPPLIBNAME})
add_library(${CPPLIBNAME} ${CPP_SRCS})

enable_testing()
add_subdirectory(tests)


install (TARGETS ${CLI_EXECUTABLE} ${OLD_CLI_EXECUTABLE}
         DESTINATION bin)
install(FILES ${CONFIG_FILENAME} DESTINATION ${CONFIG_INSTALL_DIR}
        COMPONENT config)
