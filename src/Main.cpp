#include "Application.h"

#include <glm/glm.hpp>

// Precompiled:
// glad/glad.h
// imgui.h

#include "camera/FreeflyCamera.hpp"

class SSSApp : public sss::Application {
  bool keepRunning = true;

  float avgDeltaT = 0.0f;
  float elapsed = 0.0f;
  size_t numFrames = 0;

  glm::vec3 test = glm::vec3(0.0f);

  BaseCamera* cam = nullptr;

public:
  bool init() override {
    cam = new FreeflyCamera();
    cam->setFovy(60.f);
    cam->setLookAt(Vec3f(1.f, 0.f, 0.f));
    cam->setPosition(Vec3f(0.f));
    cam->setScreenSize(1024, 576);
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

  void processEvent(const SDL_Event& p_event) {
    if (p_event.type == SDL_KEYDOWN) {
      switch (p_event.key.keysym.scancode) {
      case SDL_SCANCODE_W: // Front
        cam->moveFront();
        break;
      case SDL_SCANCODE_S: // Back
        cam->moveBack();
        break;
      case SDL_SCANCODE_A: // Left
        cam->moveLeft();
        break;
      case SDL_SCANCODE_D: // Right
        cam->moveRight();
        break;
      case SDL_SCANCODE_R: // Up
        cam->moveUp();
        break;
      case SDL_SCANCODE_F: // Down
        cam->moveDown();
        break;
      case SDL_SCANCODE_SPACE: // Print camera info
        cam->print();
        break;
      default:
        break;
      }
    }

    // Rotate when left click + motion (if not on Imgui widget).
    if (p_event.type == SDL_MOUSEMOTION &&
        p_event.motion.state & SDL_BUTTON_LMASK &&
        !ImGui::GetIO().WantCaptureMouse) {
      cam->rotate(p_event.motion.xrel,
                      p_event.motion.yrel);
    }
  }
};

int main(int, char**) {
  SSSApp app;
  sss::AppContext ctx(app);
  ctx.start("sss", 1024, 576);
  return 0;
}
