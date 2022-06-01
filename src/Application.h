#pragma once
#ifndef SSS_APPLICATION_H
#define SSS_APPLICATION_H

typedef void* SDL_GLContext;
union SDL_Event;
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

  void start(const char* name, int w, int h);

private:
  void cleanup();

  void renderUI();
  void renderFrame();

  void processEvent(const SDL_Event& e);
};

} // namespace sss

#endif
