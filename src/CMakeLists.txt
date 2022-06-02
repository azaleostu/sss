add_subdirectory(models)
add_subdirectory(shader)
add_subdirectory(camera)
add_subdirectory(utils)

target_sources(sss
  PUBLIC
  define.hpp
  Application.cpp
  Application.h
  Main.cpp)
