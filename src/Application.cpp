#include "Application.h"

#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

// Precompiled:
// SDL.h
// imgui.h
// iostream

namespace sss {

namespace {

SDL_Window* createWindow(const char* name, int w, int h) {
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  return SDL_CreateWindow(name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
                          SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
}

bool loadGL() { return gladLoadGLLoader(SDL_GL_GetProcAddress); }

} // namespace

// Automatically calls cleanup in destructor.
class AutoCleanup {
  AppContext& ctx;

public:
  explicit AutoCleanup(AppContext& ctx)
    : ctx(ctx) {}
  ~AutoCleanup() { ctx.cleanup(); }
};

void AppContext::start(const char* name, int w, int h) {
  if (running)
    return;

  AutoCleanup scope(*this);

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cout << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
    return;
  }
  SDLInitialized = true;

  window = createWindow(name, w, h);
  if (!window) {
    std::cout << "Failed to create window: " << SDL_GetError() << std::endl;
    return;
  }

  context = SDL_GL_CreateContext(window);
  if (!context) {
    std::cout << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
    return;
  }
  if (!loadGL()) {
    std::cout << "Failed to load OpenGL functions" << std::endl;
    return;
  }

  if (!initImGui()) {
    std::cout << "Failed to initialize Dear ImGui context" << std::endl;
    return;
  }

  if (!app.init()) {
    std::cout << "Failed to initialize app" << std::endl;
    return;
  }
  appInitialized = true;

  uint64_t freq = SDL_GetPerformanceFrequency();
  uint64_t t1 = SDL_GetPerformanceCounter();

  prepareWindow();
  SDL_GL_SetSwapInterval(1);

  running = true;
  while (running) {
    SDL_Event e = {};
    while (SDL_PollEvent(&e))
      processEvent(e);

    uint64_t t2 = SDL_GetPerformanceCounter();
    float deltaT = (float)(t2 - t1) / (float)freq;
    running = running && app.update(deltaT);
    t1 = t2;

    render();
  }
}

bool AppContext::initImGui() {
  IMGUI_CHECKVERSION();
  if (!ImGui::CreateContext())
    return false;
  ImGui::StyleColorsDark();

  return ImGui_ImplSDL2_InitForOpenGL(window, context) && ImGui_ImplOpenGL3_Init("#version 460");
}

void AppContext::cleanup() {
  if (appInitialized) {
    app.cleanup();
    appInitialized = false;
  }
  running = false;

  ImGui::DestroyContext();
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

  if (SDLInitialized) {
    SDL_Quit();
    SDLInitialized = false;
  }
}

void AppContext::prepareWindow() {
  SDL_SetWindowOpacity(window, 0.0f);
  SDL_ShowWindow(window);
  render();
  SDL_SetWindowOpacity(window, 1.0f);
}

void AppContext::processEvent(const SDL_Event& e) {
  if (e.type == SDL_QUIT)
    running = false;

  ImGui_ImplSDL2_ProcessEvent(&e);
  app.processEvent(e);
}

void AppContext::renderUI() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();

  ImGui::NewFrame();
  app.renderUI();
  ImGui::Render();

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void AppContext::renderFrame() {
  app.beginFrame();
  app.renderFrame();
  app.endFrame();
}

void AppContext::render() {
  renderFrame();
  renderUI();
  SDL_GL_SwapWindow(window);
}

} // namespace sss
