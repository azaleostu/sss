cmake_minimum_required(VERSION 3.21)
project(sss
  LANGUAGES C CXX)

add_executable(sss)
set_target_properties(sss
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

target_precompile_headers(sss
  PRIVATE PCH.h)

add_subdirectory(src)
add_subdirectory(vendor)

target_link_libraries(sss
  PRIVATE assimp glad SDL2main SDL2-static)
