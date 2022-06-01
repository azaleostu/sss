add_library(glad STATIC)
target_include_directories(glad PUBLIC include)

target_sources(glad
  PUBLIC
  include/glad/glad.h
  include/KHR/khrplatform.h
  PRIVATE
  src/glad.c)
