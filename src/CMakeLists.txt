target_sources(sss
  PUBLIC
  Application.cpp
  Application.h
  Main.cpp
  MathDefines.h)

add_subdirectory(models)
add_subdirectory(shader)
add_subdirectory(camera)
add_subdirectory(utils)
