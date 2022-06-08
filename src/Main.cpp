#include "Application.h"
#include "SSSConfig.h"
#include "camera/FreeflyCamera.h"
#include "model/QuadModel.h"
#include "model/TriangleMeshModel.h"
#include "shader/ShaderProgram.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Precompiled:
// glad/glad.h
// imgui.h
// iostream

using namespace sss;

const char* appName = "sss";
int appW = 1024;
int appH = 576;

const char* shadersDir = SSS_ASSET_DIR "/shaders/";

struct Light {
  Vec3f position = {};
};

struct UniformLocations {
  GLint normalMatrix = GL_INVALID_INDEX;
  GLint MVMatrix = GL_INVALID_INDEX;
  GLint viewLightPosition = GL_INVALID_INDEX;
  GLint modelMatrix = GL_INVALID_INDEX;
  GLint viewMatrix = GL_INVALID_INDEX;
  GLint projectionMatrix = GL_INVALID_INDEX;
};

class SSSApp : public Application {
public:
  SSSApp()
    : m_program(shadersDir)
    , m_FBOutput(shadersDir) {}

  bool init(SDL_Window* window, int w, int h) override {
    m_window = window;
    m_viewportW = w;
    m_viewportH = h;

    if (!updateFBs())
      return false;

    if (!initProgram())
      return false;
    if (!initFBOutputProgram())
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

    m_quad.init();
    m_light.position = Vec3f(0.0f, 0.5f, 1.0f);
    return true;
  }

  void cleanup() override {
    m_program.release();
    m_model.cleanGL();
    m_quad.release();
    m_FBOutput.release();
    releaseFBs();
  }

  bool update(float deltaT) override {
    updateAvgDeltaT(deltaT);
    return m_keepRunning;
  }

  void beginFrame() override {
    if (m_viewportNeedsUpdate) {
      glViewport(0, 0, m_viewportW, m_viewportH);
      m_viewportNeedsUpdate = false;
      if (!updateFBs()) {
        std::cout << "Failed to update FBs" << std::endl;
        m_keepRunning = false;
      }
    }
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
  }

  void renderFrame() override {
    glBindFramebuffer(GL_FRAMEBUFFER, m_mainFB);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    updateUniforms(m_program, m_loc, m_model.transformation());
    m_model.render(m_program);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTextureUnit(0, m_mainFBColorTex);
    glBindTextureUnit(1, m_mainFBDepthStencilTex);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_quad.render(m_FBOutput);

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
  }

  void endFrame() override { glDisable(GL_DEPTH_TEST); }

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
  bool initProgram() {
    m_program.init();
    if (!m_program.addShader("vertex", GL_VERTEX_SHADER, "mesh.vert") ||
        !m_program.addShader("fragment", GL_FRAGMENT_SHADER, "mesh.frag")) {
      std::cout << "Could not add shaders" << std::endl;
      return false;
    }
    if (!m_program.link())
      return false;

    // get uniform locations
    m_loc.normalMatrix = m_program.getUniformLocation("uNormalMatrix");
    m_loc.MVMatrix = m_program.getUniformLocation("uMVMatrix");
    m_loc.viewLightPosition = m_program.getUniformLocation("uLight.viewPosition");
    m_loc.modelMatrix = m_program.getUniformLocation("uModelMatrix");
    m_loc.viewMatrix = m_program.getUniformLocation("uViewMatrix");
    m_loc.projectionMatrix = m_program.getUniformLocation("uProjectionMatrix");
    return true;
  }

  bool initFBOutputProgram() {
    m_FBOutput.init();
    if (!m_FBOutput.addShader("vertex", GL_VERTEX_SHADER, "fb-quad.vert") ||
        !m_FBOutput.addShader("fragment", GL_FRAGMENT_SHADER, "fb-quad-final.frag")) {
      std::cout << "Could not add shaders" << std::endl;
      return false;
    }
    return m_FBOutput.link();
  }

  bool updateFBs() {
    releaseFBs();
    glCreateFramebuffers(1, &m_mainFB);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_mainFBColorTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_mainFBDepthStencilTex);
    glTextureStorage2D(m_mainFBColorTex, 1, GL_RGB16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_mainFBDepthStencilTex, 1, GL_DEPTH24_STENCIL8, m_viewportW, m_viewportH);

    // Sample depth as a grayscale texture.
    glTextureParameteri(m_mainFBDepthStencilTex, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);

    glNamedFramebufferTexture(m_mainFB, GL_COLOR_ATTACHMENT0, m_mainFBColorTex, 0);
    glNamedFramebufferTexture(m_mainFB, GL_DEPTH_STENCIL_ATTACHMENT, m_mainFBDepthStencilTex, 0);
    if (glCheckNamedFramebufferStatus(m_mainFB, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      std::cout << "Main FB is incomplete" << std::endl;
      return false;
    }
    return true;
  }

  void releaseFBs() {
    if (m_mainFB) {
      glDeleteTextures(1, &m_mainFBColorTex);
      glDeleteTextures(1, &m_mainFBDepthStencilTex);
      glDeleteFramebuffers(1, &m_mainFB);
      m_mainFB = 0;
      m_mainFBColorTex = 0;
      m_mainFBDepthStencilTex = 0;
    }
  }

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

  void updateUniforms(const ShaderProgram& program, const UniformLocations& loc,
                      const Mat4f& transform) const {
    const Vec4f viewPos = m_cam.viewMatrix() * Vec4f(m_light.position, 1.0f);
    program.setVec3(loc.viewLightPosition, viewPos);

    const Mat4f mv = m_cam.viewMatrix() * transform;
    program.setMat4(loc.MVMatrix, mv);
    program.setMat4(loc.normalMatrix, glm::transpose(glm::inverse(mv)));

    program.setMat4(loc.modelMatrix, transform);
    program.setMat4(loc.viewMatrix, m_cam.viewMatrix());
    program.setMat4(loc.projectionMatrix, m_cam.projectionMatrix());
  }

private:
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
  ShaderProgram m_program;
  UniformLocations m_loc;
  TriangleMeshModel m_model;
  Light m_light;

  GLuint m_mainFB = 0;
  GLuint m_mainFBColorTex = 0;
  GLuint m_mainFBDepthStencilTex = 0;
  QuadModel m_quad;
  ShaderProgram m_FBOutput;
};

int main(int argc, char** argv) {
  SSSApp app;
  sss::AppContext ctx(app);
  ctx.start(appName, appW, appH);
  return 0;
}
