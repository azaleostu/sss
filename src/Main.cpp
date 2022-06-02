#include "Application.h"

// Precompiled:
// glad/glad.h
// imgui.h

class SSSApp : public sss::Application {
  bool keepRunning = true;

public:
  bool init() override {
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    return true;
  }

  bool update() override { return keepRunning; }

  void beginFrame() override { glClear(GL_COLOR_BUFFER_BIT); }
  void beginUI() override {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::Button("Close"))
        keepRunning = false;

      ImGui::EndMainMenuBar();
    }
  }
};

int main(int, char**) {
  SSSApp app;
  sss::AppContext ctx(app);
  ctx.start("sss", 1600, 900);
  return 0;
}
