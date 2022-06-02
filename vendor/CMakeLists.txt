set(BUILD_SHARED_LIBS OFF CACHE BOOL
  "Build shared libraries.")

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL
  "Build Assimp tools.")
set(ASSIMP_BUILD_DOCS OFF CACHE BOOL
  "Build Assimp docs.")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL
  "Build Assimp tests.")
set(ASSIMP_INSTALL OFF CACHE BOOL
  "Generate install target for Assimp.")
set(ASSIMP_NO_EXPORT ON CACHE BOOL
  "Disable Assimp's export functionality.")
add_subdirectory(assimp)

add_subdirectory(glad)

set(SDL2_DISABLE_SDL2MAIN OFF CACHE BOOL
  "Disable SDL2main target.")
set(SDL2_DISABLE_INSTALL ON CACHE BOOL
  "Disable SDL2 install target.")
set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL
  "Disable SDL2 uninstall target.")
set(SDL_SHARED OFF CACHE BOOL
  "Build a shared version of SDL2.")
set(SDL_STATIC ON CACHE BOOL
  "Build a static version of SDL2.")
add_subdirectory(SDL)

include(ImGuiTarget.cmake)
