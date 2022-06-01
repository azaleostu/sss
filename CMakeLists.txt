cmake_minimum_required(VERSION 3.21)
project(sss LANGUAGES CXX)

add_executable(sss)
set_target_properties(sss
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
add_subdirectory(vendor)
