#pragma once
#ifndef SSS_APPLICATION_H
#define SSS_APPLICATION_H

#include <cstddef>
#include <cstdint>

#if INTPTR_MAX != INT64_MAX
#error "build target must be 64 bits"
#endif

union SDL_Event;
typedef void* SDL_GLContext;
struct SDL_Window;

namespace sss {

class Application {
public:
  virtual ~Application() = default;

  virtual bool init(SDL_Window* window, int w, int h) { return true; }
  virtual void cleanup() {}

  virtual bool update(float deltaT) { return true; }

  virtual void beginFrame() {}
  virtual void endFrame() {}
  virtual void renderFrame() {}
  virtual void renderUI() {}

  virtual void processEvent(const SDL_Event& e) {}
};

class AppContext {
public:
  friend class AutoCleanup;

  explicit AppContext(Application& app)
    : m_app(app) {}

  AppContext(const AppContext&) = delete;
  AppContext& operator=(const AppContext&) = delete;
  AppContext(AppContext&&) = delete;
  AppContext& operator=(AppContext&&) = delete;

  void start(const char* name, int w, int h);

private:
  bool initImGui();
  void cleanup();
  void processEvent(const SDL_Event& e);
  void renderUI();

private:
  Application& m_app;

  SDL_Window* m_window = nullptr;
  SDL_GLContext m_context = nullptr;

  bool m_running = false;
  bool m_SDLInitialized = false;
};

} // namespace sss

#endif
