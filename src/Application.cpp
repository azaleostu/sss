#include "Application.h"

#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

// Precompiled:
// SDL.h
// imgui.h
// iostream

namespace sss {

// Automatically calls cleanup in destructor.
class AutoCleanup {
  AppContext& ctx;

public:
  explicit AutoCleanup(AppContext& ctx)
    : ctx(ctx) {}
  ~AutoCleanup() { ctx.cleanup(); }
};

void AppContext::start(const char* name, int w, int h) {
  if (m_running)
    return;

  AutoCleanup scope(*this);

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cout << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
    return;
  }
  m_SDLInitialized = true;

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  m_window = SDL_CreateWindow(name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  if (!m_window) {
    std::cout << "Failed to create window: " << SDL_GetError() << std::endl;
    return;
  }

  m_context = SDL_GL_CreateContext(m_window);
  if (!m_context) {
    std::cout << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
    return;
  }
  if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
    std::cout << "Failed to load OpenGL functions" << std::endl;
    return;
  }

  if (!initImGui()) {
    std::cout << "Failed to initialize Dear ImGui context" << std::endl;
    return;
  }

  if (!m_app.init(m_window, w, h)) {
    std::cout << "Failed to initialize app" << std::endl;
    return;
  }

  uint64_t freq = SDL_GetPerformanceFrequency();
  uint64_t t1 = SDL_GetPerformanceCounter();

  SDL_ShowWindow(m_window);
  SDL_GL_SetSwapInterval(1);

  m_running = true;
  while (m_running) {
    SDL_Event e = {};
    while (SDL_PollEvent(&e))
      processEvent(e);

    uint64_t t2 = SDL_GetPerformanceCounter();
    float deltaT = (float)(t2 - t1) / (float)freq;
    m_running = m_running && m_app.update(deltaT);
    t1 = t2;

    m_app.beginFrame();
    m_app.renderFrame();
    m_app.endFrame();

    renderUI();
    SDL_GL_SwapWindow(m_window);
  }
}

bool AppContext::initImGui() {
  IMGUI_CHECKVERSION();
  if (!ImGui::CreateContext())
    return false;
  ImGui::StyleColorsDark();

  return ImGui_ImplSDL2_InitForOpenGL(m_window, m_context) &&
         ImGui_ImplOpenGL3_Init("#version 460");
}

void AppContext::cleanup() {
  m_app.cleanup();
  m_running = false;

  ImGui::DestroyContext();
  SDL_GL_MakeCurrent(nullptr, nullptr);
  if (m_context) {
    SDL_GL_DeleteContext(m_context);
    m_context = nullptr;
  }

  if (m_window) {
    SDL_HideWindow(m_window);
    SDL_DestroyWindow(m_window);
    m_window = nullptr;
  }

  if (m_SDLInitialized) {
    SDL_Quit();
    m_SDLInitialized = false;
  }
}

void AppContext::processEvent(const SDL_Event& e) {
  if (e.type == SDL_QUIT)
    m_running = false;

  ImGui_ImplSDL2_ProcessEvent(&e);
  m_app.processEvent(e);
}

void AppContext::renderUI() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();

  ImGui::NewFrame();
  m_app.renderUI();
  ImGui::Render();

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace sss
