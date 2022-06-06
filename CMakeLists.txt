cmake_minimum_required(VERSION 3.21)
project(sss
  LANGUAGES C CXX)

add_executable(sss)
set_target_properties(sss
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

target_precompile_headers(sss
  PRIVATE PCH.h)

set(SSS_ASSET_DIR ${CMAKE_SOURCE_DIR}/assets)
set(SSS_SHIPPING OFF)
configure_file(Config.h.in include/SSSConfig.h @ONLY)
target_include_directories(sss PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

add_subdirectory(src)
add_subdirectory(vendor)

target_link_libraries(sss
  PRIVATE assimp glad glm ImGui SDL2main SDL2-static stb)
