add_library(stb INTERFACE)
target_include_directories(stb INTERFACE stb/include)

target_sources(stb PUBLIC stb/include/stb_image.h)
