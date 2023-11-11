cmake_minimum_required(VERSION 3.22)
project("Gtest")

find_package(GTest REQUIRED)

add_executable(a.out ./test/test.cpp)
target_link_libraries(a.out ${GTEST_LIBRARIES})

#ctest
enable_testing()
add_test(add a.out)