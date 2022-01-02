## tests

find_package(GTest REQUIRED)
find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS filesystem system REQUIRED)
set(LIBS ${LIBS} boost_filesystem boost_system)

set(TEST_RUNNER ${PROJECT_NAME}_tests)
include_directories(${INCLUDE_DIRECTORIES} ${GTEST_INCLUDE_DIRS} ${PROJECT_INCLUDE_DIR})
file(GLOB TESTS test_*.cpp)  # all test source files in the current directory

add_test(${TEST_RUNNER} ${TEST_RUNNER})
add_executable(${TEST_RUNNER} "testrunner.cpp" ${TESTS} common.cpp)
target_link_libraries(${TEST_RUNNER} ${CLIBNAME} ${CPPLIBNAME}
  ${GTEST_BOTH_LIBRARIES}
  pthread
  confuse
  ${LIBS})
