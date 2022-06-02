#include "Application.h"

// Precompiled:
// glad/glad.h
// imgui.h

class SSSApp : public sss::Application {
  bool keepRunning = true;

  float avgDeltaT = 0.0f;
  float elapsed = 0.0f;
  size_t numFrames = 0;

public:
  bool init() override {
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    return true;
  }

  bool update(float deltaT) override {
    updateAvgDeltaT(deltaT);
    return keepRunning;
  }

  void beginFrame() override { glClear(GL_COLOR_BUFFER_BIT); }

  void renderUI() override {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::Button("Close"))
        keepRunning = false;
      ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Stats");
    ImGui::Text("Average %.2ffps", 1 / avgDeltaT);
    ImGui::End();
  }

private:
  void updateAvgDeltaT(float deltaT) {
    ++numFrames;
    elapsed += deltaT;
    if (elapsed > 0.075f) {
      avgDeltaT = elapsed / (float)numFrames;
      elapsed = 0.0f;
      numFrames = 0;
    }
  }
};

int main(int, char**) {
  SSSApp app;
  sss::AppContext ctx(app);
  ctx.start("sss", 1600, 900);
  return 0;
}
