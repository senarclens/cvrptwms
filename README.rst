================================================
 Solver for VRPTW with multiple service workers
================================================
This software solves vehicle routing problems with time windows and
multiple service workers. For information about the problem itself, consult
http://senarclens.eu/~gerald/research/


Disclaimer & License
====================
This software is free software. It is licensed under the terms of the GNU
General Public License version 3. See the file COPYING for copying
conditions.

As I do not sell this software, it is provided without warranty,
without even implied warranty of merchantability, satisfactoriness or
fitness for any particular use. It is left to the user to check if the
output of this program is correct.


Installation
============
This package depends on
- cmake
- libconfuse-dev
- libboost-filesystem-dev (>= 1.53)
- libboost-program-options-dev (>= 1.53)

On debian-based systems, run
$ sudo apt-get install cmake libconfuse-dev libboost-filesystem-dev \
  libboost-program-options-dev

To build the documentation (make doc) you also need to install doxygen and
graphviz.
$ sudo apt-get install doxygen graphviz

Running the tests requires installation of googletest (see below).


Building the solver
-------------------
After installing all requirements as listed above, build the solver with
$ make release
For creating the documentation, run
$ make doc
The entry point to the source code documentation is then made available at
./doc/html/index.html

Currently, parts of the program are rewritten in C++ and others extended. To
use the newer parts, install
$ sudo apt-get install g++ cmake
and compile via
$ mkdir build && cd build
$ cmake ..
$ make -j3
$ make install


Running the solver
==================
To get help for running the solver, type
$ ./vrptwms --help
The Solomon benchmark data files, formatted in a way this solver understands,
are available in the subdirectory ./data.


Testing
=======
For testing this package, you need the Google C++ Testing Framework,
cmake and a c++ compiler. A static syntax analysis of the entire project is
run whenever the tests are built. These require cppcheck.

To set the requirements uo on debian-based systems, run
$ sudo apt-get install cmake libgtest-dev cppcheck
$ cd /usr/src/gtest
$ sudo cmake CMakeLists.txt
$ sudo make
$ sudo cp *.a /usr/lib
$ cd -

To build and execute the tests enter this project's directory and type
$ make tests
$ tests/testrunner

Development
===========
In order to create core files, edit /etc/security/limits.conf to contain

#<domain>      <type>  <item>         <value>
$USER          soft    core           50000

which allows the creation of core files of up to 50MB.

In order to profile the code, first make a build suitable for profiling
$ make clean
$ make profile
Then make a run that contains the functions you want to profile. When done,
a profiling output file "gmon.out" can be found in the working directory.
Analyze this file using the gprof command (without any arguments to the
vrptwms executable).
$ gprof ./vrptwms gmon.out

IMPORTANT:
Apparently, there is still a bug in the Makefile - changes in the config
struct lead to a segfault when not completely recompiling. => I need to either
correct the issue or switch to cmake.

Author
======
The program was written by Gerald Senarclens de Grancy <oss@senarclens.eu>.
