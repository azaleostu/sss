add_library(ImGui STATIC)
target_link_libraries(ImGui PUBLIC SDL2-static)

target_include_directories(ImGui
  PUBLIC
  imgui
  imgui/backends)

target_sources(ImGui
  PUBLIC
  imgui/backends/imgui_impl_opengl3.h
  imgui/backends/imgui_impl_sdl.h
  imgui/imconfig.h
  imgui/imgui.h
  imgui/imstb_rectpack.h
  imgui/imstb_textedit.h
  imgui/imstb_truetype.h
  PRIVATE
  imgui/backends/imgui_impl_opengl3.cpp
  imgui/backends/imgui_impl_sdl.cpp
  imgui/imgui.cpp
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_internal.h
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp)
