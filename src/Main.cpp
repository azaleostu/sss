#include <SDL.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <glad/glad.h>

#if INTPTR_MAX != INT64_MAX
#error "build target must be 64 bits"
#endif

enum {
  Success = 0,
  SDLError,
  OpenGLLoaderError,
};

SDL_Window* window = nullptr;
SDL_GLContext context = nullptr;
bool running = false;

void cleanup() {
  running = false;
  if (context) {
    SDL_GL_MakeCurrent(window, nullptr);
    SDL_GL_DeleteContext(context);
    context = nullptr;
  }
  if (window) {
    SDL_HideWindow(window);
    SDL_DestroyWindow(window);
    window = nullptr;
  }
  SDL_Quit();
}

SDL_Window* createWindow(int w, int h) {
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  return SDL_CreateWindow("sss", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                          w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
}

bool loadGLFunctions() { return gladLoadGLLoader(SDL_GL_GetProcAddress); }

void processEvent(const SDL_Event& e) {
  if (e.type == SDL_QUIT)
    running = false;
}
void processEvents() {
  SDL_Event e = {};
  while (SDL_PollEvent(&e))
    processEvent(e);
}

int main(int, char**) {
  atexit(cleanup);
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("failed to initialise SDL2: %s\n", SDL_GetError());
    exit(SDLError);
  }

  window = createWindow(1600, 900);
  if (!window) {
    printf("failed to create window: %s\n", SDL_GetError());
    exit(SDLError);
  }

  context = SDL_GL_CreateContext(window);
  if (!context) {
    printf("failed to create OpenGL context: %s\n", SDL_GetError());
    exit(SDLError);
  }
  SDL_GL_MakeCurrent(window, context);
  if (!loadGLFunctions()) {
    printf("failed to load OpenGL functions\n");
    exit(OpenGLLoaderError);
  }

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  SDL_ShowWindow(window);
  running = true;
  while (running) {
    processEvents();
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(window);
  }
  exit(Success);
}
