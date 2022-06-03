#include "Application.h"

#include "glm/gtc/type_ptr.hpp"
#include <glm/glm.hpp>

// Precompiled:
// glad/glad.h
// imgui.h

#include "camera/FreeflyCamera.hpp"
#include "model/triangleMeshModel.hpp"
#include "shader/ShaderManager.hpp"

class SSSApp : public sss::Application {
  bool keepRunning = true;

  float avgDeltaT = 0.0f;
  float elapsed = 0.0f;
  size_t numFrames = 0;

  glm::vec3 test = glm::vec3(0.0f);

  BaseCamera& cam = ffCam;
  FreeflyCamera ffCam;
  GLuint program = GL_INVALID_INDEX;
  ShaderManager sm;
  TriangleMeshModel model;

  struct Locations {
    GLint _uMVPMatrixLoc = GL_INVALID_INDEX;
    GLint _uNormalMatrixLoc = GL_INVALID_INDEX;
    GLint _uMVMatrixLoc = GL_INVALID_INDEX;
    GLint _uMMatrixLoc = GL_INVALID_INDEX;
    GLint _uViewLightPositionLoc = GL_INVALID_INDEX;
    GLint _uModelMatrixLoc = GL_INVALID_INDEX;
    GLint _uViewMatrixLoc = GL_INVALID_INDEX;
    GLint _uProjectionMatrixLoc = GL_INVALID_INDEX;
  } loc;

public:
  SSSApp() : sm("../../../../src/shaders/") {}

  bool init() override {
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    // init program
    initProgram();
    // init camera
    cam.setFovy(60.f);
    cam.setLookAt(Vec3f(1.f, 0.f, 0.f));
    cam.setPosition(Vec3f(0.f));
    cam.setScreenSize(1024, 576);
    // init models
    model.load("bunny", "../../../../src/models/bunny/bunny.obj");
    return true;
  }

  void initProgram() {
    // init shadermanager and shaders
    sm.init();
    sm.addShader("vertex", GL_VERTEX_SHADER, "mesh.vert");
    sm.addShader("fragment", GL_FRAGMENT_SHADER, "mesh.frag");
    sm.getShader("vertex")->compile();
    sm.getShader("fragment")->compile();
    sm.link();
    sm.use(program);
    sm.shaders.clear();
    // get uniform locations
    loc._uNormalMatrixLoc = glGetUniformLocation(program, "uNormalMatrix");
    loc._uMVPMatrixLoc = glGetUniformLocation(program, "uMVPMatrix");
    loc._uMVMatrixLoc = glGetUniformLocation(program, "uMVMatrix");
    loc._uViewLightPositionLoc = glGetUniformLocation(program, "uViewLightPos");
    loc._uModelMatrixLoc = glGetUniformLocation(program, "uModelMatrix");
    loc._uViewMatrixLoc = glGetUniformLocation(program, "uViewMatrix");
    loc._uProjectionMatrixLoc =
        glGetUniformLocation(program, "uProjectionMatrix");
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

  void renderFrame() override {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // use basic shaders
    sm.use(program);
    // do stuff ...
    renderModel(model);
  }

  void updateMatrices(const Mat4f& model, const Mat4f& view,
                      const Mat4f& projection) {
    glProgramUniformMatrix4fv(program, loc._uModelMatrixLoc, 1, false,
                              glm::value_ptr(model));
    glProgramUniformMatrix4fv(program, loc._uViewMatrixLoc, 1, false,
                              glm::value_ptr(view));
    glProgramUniformMatrix4fv(program, loc._uProjectionMatrixLoc, 1, false,
                              glm::value_ptr(projection));
  }

  void updateuMVPmatrix() {
    glProgramUniformMatrix4fv(program, loc._uMVPMatrixLoc, 1, false,
                              glm::value_ptr(cam.getUMVPMatrix()));
  }

  void updateuMVMatrix() {
    glProgramUniformMatrix4fv(program, loc._uMVMatrixLoc, 1, false,
                              glm::value_ptr(cam.getUMVMatrix()));
  }

  void updateuMMatrix(const TriangleMeshModel& model) {
    glProgramUniformMatrix4fv(program, loc._uMMatrixLoc, 1, false,
                              glm::value_ptr(model._transformation));
  }

  void updateuNormalMatrix() {
    glProgramUniformMatrix4fv(
        program, loc._uNormalMatrixLoc, 1, false,
        glm::value_ptr(glm::transpose(glm::inverse(cam.getUMVMatrix()))));
  }

  void renderModel(TriangleMeshModel& tmm) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    cam._computeUMVPMatrix(tmm._transformation);
    updateuMVPmatrix();
    cam._computeUMVMatrix(tmm._transformation);
    updateuMVMatrix();
    updateuMMatrix(tmm);
    updateuNormalMatrix();
    updateMatrices(tmm._transformation, cam.getViewMatrix(),
                   cam.getProjectionMatrix());
    tmm.update();
    tmm.render(program);
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
      case SDL_SCANCODE_W: // Front
        cam.moveFront();
        break;
      case SDL_SCANCODE_S: // Back
        cam.moveBack();
        break;
      case SDL_SCANCODE_A: // Left
        cam.moveLeft();
        break;
      case SDL_SCANCODE_D: // Right
        cam.moveRight();
        break;
      case SDL_SCANCODE_R: // Up
        cam.moveUp();
        break;
      case SDL_SCANCODE_F: // Down
        cam.moveDown();
        break;
      case SDL_SCANCODE_SPACE: // Print camera info
        cam.print();
        break;
      default:
        break;
      }
    }

    // Rotate when left click + motion (if not on Imgui widget).
    if (e.type == SDL_MOUSEMOTION && e.motion.state & SDL_BUTTON_LMASK &&
        !ImGui::GetIO().WantCaptureMouse) {
      cam.rotate((float)e.motion.xrel, (float)e.motion.yrel);
    }
  }
};

int main(int, char**) {
  SSSApp app;
  sss::AppContext ctx(app);
  ctx.start("sss", 1024, 576);
  return 0;
}
