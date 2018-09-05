cmake_minimum_required (VERSION 2.8)
project (tinyLMP_test)

add_definitions (-std=c++11)

include_directories(inc)
aux_source_directory (test DIR_SRCS)
add_executable(test ${DIR_SRCS})