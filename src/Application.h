#pragma once
#ifndef SSS_APPLICATION_H
#define SSS_APPLICATION_H

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

  virtual bool init() { return true; }
  virtual void cleanup() {}

  virtual void beginUI() {}
  virtual void endUI() {}
  virtual void beginFrame() {}
  virtual void endFrame() {}

  virtual void renderUI() {}
  virtual void renderFrame() {}

  virtual void processEvent(const SDL_Event& e) {}
};

class AppContext {
  friend class AutoCleanup;

  Application& app;

  SDL_Window* window = nullptr;
  SDL_GLContext context = nullptr;
  bool running = false;

public:
  explicit AppContext(Application& app) : app(app) {}

  AppContext(const AppContext&) = delete;
  AppContext& operator=(const AppContext&) = delete;
  AppContext(AppContext&&) = delete;
  AppContext& operator=(AppContext&&) = delete;

  void start(const char* name, int w, int h);

private:
  void cleanup();

  void renderUI();
  void renderFrame();

  void processEvent(const SDL_Event& e);
};

} // namespace sss

#endif
