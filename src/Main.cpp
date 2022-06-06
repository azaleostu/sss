#include "Application.h"
#include "SSSConfig.h"
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

const char* appName = "sss";
int appW = 1024;
int appH = 576;

class SSSApp : public Application {
  bool m_keepRunning = true;
  SDL_Window* m_window = nullptr;

  GLsizei m_viewportW = 0;
  GLsizei m_viewportH = 0;
  bool m_viewportNeedsUpdate = false;

  float m_avgDeltaT = 0.0f;
  float m_elapsed = 0.0f;
  size_t m_numFrames = 0;

  BaseCamera& m_cam = m_ffCam;
  FreeflyCamera m_ffCam;
  GLuint m_program = GL_INVALID_INDEX;
  ShaderManager m_sm;
  TriangleMeshModel m_model;

  struct Locations {
    GLint normalMatrix = GL_INVALID_INDEX;
    GLint MVMatrix = GL_INVALID_INDEX;
    GLint viewLightPosition = GL_INVALID_INDEX;
    GLint modelMatrix = GL_INVALID_INDEX;
    GLint viewMatrix = GL_INVALID_INDEX;
    GLint projectionMatrix = GL_INVALID_INDEX;
  } m_loc;

public:
  SSSApp()
    : m_sm(SSS_ASSET_DIR "/shaders/") {}

  bool init(SDL_Window* window, int w, int h) override {
    m_window = window;
    m_viewportW = w;
    m_viewportH = h;

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    if (!initProgram())
      return false;

    // init camera
    m_cam.setFovy(60.f);
    m_cam.setLookAt(Vec3f(1.f, 0.f, 0.f));
    m_cam.setPosition(Vec3f(0.f, 0.f, 0.f));
    m_cam.setScreenSize(appW, appH);
    m_cam.setSpeed(0.05f);

    // init models
    m_model.load("bunny", SSS_ASSET_DIR "/models/james/james_hi.obj");
    m_model.setTransformation(glm::scale(m_model.transformation(), Vec3f(0.01f)));
    return true;
  }

  bool initProgram() {
    m_sm.init();
    if (!m_sm.addShader("vertex", GL_VERTEX_SHADER, "mesh.vert") ||
        !m_sm.addShader("fragment", GL_FRAGMENT_SHADER, "mesh.frag")) {
      std::cout << "Could not add shaders" << std::endl;
      return false;
    }
    m_sm.getShader("vertex")->compile();
    m_sm.getShader("fragment")->compile();
    m_sm.link();
    m_sm.use(m_program);

    // get uniform locations
    m_loc.normalMatrix = glGetUniformLocation(m_program, "uNormalMatrix");
    m_loc.MVMatrix = glGetUniformLocation(m_program, "uMVMatrix");
    m_loc.viewLightPosition = glGetUniformLocation(m_program, "uViewLightPos");
    m_loc.modelMatrix = glGetUniformLocation(m_program, "uModelMatrix");
    m_loc.viewMatrix = glGetUniformLocation(m_program, "uViewMatrix");
    m_loc.projectionMatrix = glGetUniformLocation(m_program, "uProjectionMatrix");
    return true;
  }

  bool update(float deltaT) override {
    updateAvgDeltaT(deltaT);
    return m_keepRunning;
  }

  void beginFrame() override {
    if (m_viewportNeedsUpdate) {
      glViewport(0, 0, m_viewportW, m_viewportH);
      m_viewportNeedsUpdate = false;
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void renderFrame() override {
    m_sm.use(m_program);
    updateUniforms(m_model);
    m_model.render(m_program);
  }

  void renderUI() override {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::Button("Close"))
        m_keepRunning = false;
      ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Config");
    ImGui::Text("Average %.2ffps", 1 / m_avgDeltaT);
    ImGui::End();
  }

private:
  void updateAvgDeltaT(float deltaT) {
    ++m_numFrames;
    m_elapsed += deltaT;
    if (m_elapsed > 0.075f) {
      m_avgDeltaT = m_elapsed / (float)m_numFrames;
      m_elapsed = 0.0f;
      m_numFrames = 0;
    }
  }

  void processEvent(const SDL_Event& e) override {
    if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
      m_viewportW = e.window.data1;
      m_viewportH = e.window.data2;
      m_viewportNeedsUpdate = true;
    }

    processKeyEvent(e);
    processMouseEvent(e);
  }

  void processKeyEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
      switch (e.key.keysym.scancode) {
      case SDL_SCANCODE_W:
        m_cam.moveFront();
        break;
      case SDL_SCANCODE_S:
        m_cam.moveBack();
        break;
      case SDL_SCANCODE_A:
        m_cam.moveLeft();
        break;
      case SDL_SCANCODE_D:
        m_cam.moveRight();
        break;
      case SDL_SCANCODE_R:
        m_cam.moveUp();
        break;
      case SDL_SCANCODE_F:
        m_cam.moveDown();
        break;
      case SDL_SCANCODE_SPACE:
        m_cam.print();
        break;
      default:
        break;
      }
    }
  }

  void processMouseEvent(const SDL_Event& e) {
    if (ImGui::GetIO().WantCaptureMouse)
      return;

    if (e.type == SDL_MOUSEMOTION && e.motion.state & SDL_BUTTON_LMASK)
      m_cam.rotate((float)e.motion.xrel, (float)e.motion.yrel);

    if (e.type == SDL_MOUSEWHEEL)
      m_cam.setFovy(m_cam.fovy() - (float)e.wheel.y);
  }

  void updateUniforms(const TriangleMeshModel& m) const {
    const Vec4f viewPos = m_cam.viewMatrix() * Vec4f(m_cam.position(), 1.0f);
    glProgramUniform3fv(m_program, m_loc.viewLightPosition, 1, glm::value_ptr(viewPos));

    const Mat4f mv = m_cam.viewMatrix() * m.transformation();
    glProgramUniformMatrix4fv(m_program, m_loc.MVMatrix, 1, false, glm::value_ptr(mv));
    glProgramUniformMatrix4fv(m_program, m_loc.normalMatrix, 1, false,
                              glm::value_ptr(glm::transpose(glm::inverse(mv))));

    glProgramUniformMatrix4fv(m_program, m_loc.modelMatrix, 1, false,
                              glm::value_ptr(m.transformation()));
    glProgramUniformMatrix4fv(m_program, m_loc.viewMatrix, 1, false,
                              glm::value_ptr(m_cam.viewMatrix()));
    glProgramUniformMatrix4fv(m_program, m_loc.projectionMatrix, 1, false,
                              glm::value_ptr(m_cam.projectionMatrix()));
  }
};

int main(int argc, char** argv) {
  SSSApp app;
  sss::AppContext ctx(app);
  ctx.start(appName, appW, appH);
  return 0;
}
