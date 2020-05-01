cmake_minimum_required(VERSION 3.11)
project(ECM LANGUAGES C VERSION 1.0)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_library(ecm_common OBJECT src/common.c)

add_executable(ecm
	src/ecm.c
)
target_link_libraries(ecm ecm_common)

add_executable(unecm
	src/unecm.c
)
target_link_libraries(unecm ecm_common)