#include "Application.h"
#include "camera/FreeflyCamera.h"
#include "model/TriangleMeshModel.h"
#include "shader/ShaderManager.h"

#include "glm/gtc/type_ptr.hpp"
#include <glm/glm.hpp>

// Precompiled:
// glad/glad.h
// imgui.h
// iostream

using namespace sss;

class SSSApp : public Application {
  bool keepRunning = true;

  float avgDeltaT = 0.0f;
  float elapsed = 0.0f;
  size_t numFrames = 0;

  BaseCamera& cam = ffCam;
  FreeflyCamera ffCam;
  GLuint program = GL_INVALID_INDEX;
  ShaderManager sm;
  TriangleMeshModel model;

  struct Locations {
    GLint MVPMatrix = GL_INVALID_INDEX;
    GLint NormalMatrix = GL_INVALID_INDEX;
    GLint MVMatrix = GL_INVALID_INDEX;
    GLint MMatrix = GL_INVALID_INDEX;
    GLint ViewLightPosition = GL_INVALID_INDEX;
    GLint ModelMatrix = GL_INVALID_INDEX;
    GLint ViewMatrix = GL_INVALID_INDEX;
    GLint ProjectionMatrix = GL_INVALID_INDEX;
  } loc;

public:
  SSSApp()
    : sm("../../../../src/shaders/") {}

  bool init() override {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    if (!initProgram())
      return false;

    // init camera
    cam.setFovy(60.f);
    cam.setLookAt(Vec3f(1.f, 0.f, 0.f));
    cam.setPosition(Vec3f(0.f, 0.f, 0.f));
    cam.setScreenSize(1024, 576);
    cam.setSpeed(0.05f);

    // init models
    model.load("bunny", "../../../../src/models/bunny/bunny.obj");
    model.setTransformation(glm::scale(model.transformation(), Vec3f(1.0f)));
    return true;
  }

  bool initProgram() {
    sm.init();
    if (!sm.addShader("vertex", GL_VERTEX_SHADER, "mesh.vert") ||
        !sm.addShader("fragment", GL_FRAGMENT_SHADER, "mesh.frag")) {
      std::cout << "Could not add shaders" << std::endl;
      return false;
    }
    sm.getShader("vertex")->compile();
    sm.getShader("fragment")->compile();
    sm.link();
    sm.use(program);
    // get uniform locations
    loc.NormalMatrix = glGetUniformLocation(program, "uNormalMatrix");
    loc.MVPMatrix = glGetUniformLocation(program, "uMVPMatrix");
    loc.MVMatrix = glGetUniformLocation(program, "uMVMatrix");
    loc.ViewLightPosition = glGetUniformLocation(program, "uViewLightPos");
    loc.ModelMatrix = glGetUniformLocation(program, "uModelMatrix");
    loc.ViewMatrix = glGetUniformLocation(program, "uViewMatrix");
    loc.ProjectionMatrix = glGetUniformLocation(program, "uProjectionMatrix");
    return true;
  }

  bool update(float deltaT) override {
    updateAvgDeltaT(deltaT);
    return keepRunning;
  }

  void beginFrame() override { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
  void renderFrame() override {
    // use basic shaders
    sm.use(program);
    // do stuff ...
    renderModel(model);
  }

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

  void updateMatrices(const Mat4f& modelMat, const Mat4f& view, const Mat4f& projection) const {
    glProgramUniformMatrix4fv(program, loc.ModelMatrix, 1, false, glm::value_ptr(modelMat));
    glProgramUniformMatrix4fv(program, loc.ViewMatrix, 1, false, glm::value_ptr(view));
    glProgramUniformMatrix4fv(program, loc.ProjectionMatrix, 1, false, glm::value_ptr(projection));
  }

  void updateMVPMatrix() {
    glProgramUniformMatrix4fv(program, loc.MVPMatrix, 1, false, glm::value_ptr(cam.MVPMatrix()));
  }

  void updateMVMatrix() {
    glProgramUniformMatrix4fv(program, loc.MVMatrix, 1, false, glm::value_ptr(cam.MVMatrix()));
  }

  void updateMMatrix(const TriangleMeshModel& m) {
    glProgramUniformMatrix4fv(program, loc.MMatrix, 1, false,
                              glm::value_ptr(model.transformation()));
  }

  void updateNormalMatrix() {
    glProgramUniformMatrix4fv(program, loc.NormalMatrix, 1, false,
                              glm::value_ptr(glm::transpose(glm::inverse(cam.MVMatrix()))));
  }

  void renderModel(const TriangleMeshModel& m) {
    cam.computeMVPMatrix(m.transformation());
    updateMVPMatrix();
    cam.computeMVMatrix(m.transformation());
    updateMVMatrix();
    updateMMatrix(m);
    updateNormalMatrix();
    updateMatrices(m.transformation(), cam.viewMatrix(), cam.projectionMatrix());
    m.render(program);
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

  void processEvent(const SDL_Event& e) override {
    if (e.type == SDL_KEYDOWN) {
      switch (e.key.keysym.scancode) {
      case SDL_SCANCODE_W:
        cam.moveFront();
        break;
      case SDL_SCANCODE_S:
        cam.moveBack();
        break;
      case SDL_SCANCODE_A:
        cam.moveLeft();
        break;
      case SDL_SCANCODE_D:
        cam.moveRight();
        break;
      case SDL_SCANCODE_R:
        cam.moveUp();
        break;
      case SDL_SCANCODE_F:
        cam.moveDown();
        break;
      case SDL_SCANCODE_SPACE:
        cam.print();
        break;
      default:
        break;
      }
    }

    // Rotate when left click + motion (if not on Imgui widget).
    if (e.type == SDL_MOUSEMOTION && e.motion.state & SDL_BUTTON_LMASK &&
        !ImGui::GetIO().WantCaptureMouse)
      cam.rotate((float)e.motion.xrel, (float)e.motion.yrel);

    // Rotate when left click + motion (if not on Imgui widget).
    if (e.type == SDL_MOUSEWHEEL && !ImGui::GetIO().WantCaptureMouse)
      cam.setFovy(cam.fovy() - (float)e.wheel.y);
  }
};

int main(int, char**) {
  SSSApp app;
  sss::AppContext ctx(app);
  ctx.start("sss", 1024, 576);
  return 0;
}
