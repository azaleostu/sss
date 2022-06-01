#include "Application.h"

#include <cstdio>

// Precompiled:
// SDL.h

namespace sss {

namespace {

SDL_Window* createWindow(const char* name, int w, int h) {
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  return SDL_CreateWindow(name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                          w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
}

bool loadGL() { return gladLoadGLLoader(SDL_GL_GetProcAddress); }

} // namespace

// Automatically calls cleanup in destructor.
class AutoCleanup {
  AppContext& ctx;

public:
  explicit AutoCleanup(AppContext& ctx) : ctx(ctx) {}
  ~AutoCleanup() { ctx.cleanup(); }
};

void AppContext::start(const char* name, int w, int h) {
  AutoCleanup scope(*this);

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("failed to initialise SDL2: %s\n", SDL_GetError());
    return;
  }

  window = createWindow(name, w, h);
  if (!window) {
    printf("failed to create window: %s\n", SDL_GetError());
    return;
  }

  context = SDL_GL_CreateContext(window);
  if (!context) {
    printf("failed to create OpenGL context: %s\n", SDL_GetError());
    return;
  }
  if (!loadGL()) {
    printf("failed to load OpenGL functions\n");
    return;
  }

  app.init();
  SDL_ShowWindow(window);

  running = true;
  while (running) {
    SDL_Event e = {};
    while (SDL_PollEvent(&e))
      processEvent(e);

    renderFrame();
    renderUI();
    SDL_GL_SwapWindow(window);
  }
}

void AppContext::cleanup() {
  running = false;
  app.cleanup();

  SDL_GL_MakeCurrent(nullptr, nullptr);
  if (context) {
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

void AppContext::renderUI() {
  app.beginUI();
  app.renderUI();
  app.endUI();
}

void AppContext::renderFrame() {
  app.beginFrame();
  app.renderFrame();
  app.endFrame();
}

void AppContext::processEvent(const SDL_Event& e) {
  if (e.type == SDL_QUIT)
    running = false;

  app.processEvent(e);
}

} // namespace sss
