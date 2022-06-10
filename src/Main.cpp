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
int appW = 1400;
int appH = 800;

const char* shadersDir = SSS_ASSET_DIR "/shaders/";
GLsizei shadowRes = 1024;

struct Light {
  Vec3f position = {};
  Vec3f direction = {};
  float near = 0.5f;
  float far = 15.0f;
  float fovy = 45.0f;
  Mat4f view = {};
  Mat4f proj = {};

  Light() { updateMatrices(); }
  void updateMatrices() {
    const Vec3f up(0.0f, 1.0f, 0.0f);
    view = glm::lookAt(position, position + direction, up);
    proj = glm::perspective(glm::radians(fovy), 1.0f, near, far);
  }
};

struct BlurUniforms {
  GLint fovy = GL_INVALID_INDEX;
  GLint sssWidth = GL_INVALID_INDEX;
};

struct ShadowUniforms {
  GLint lightMVP = GL_INVALID_INDEX;
};

struct LightUniforms {
  GLint position = GL_INVALID_INDEX;
  GLint direction = GL_INVALID_INDEX;
  GLint farPlane = GL_INVALID_INDEX;
};

struct UniformLocations {
  GLint modelMatrix = GL_INVALID_INDEX;
  GLint MVPMatrix = GL_INVALID_INDEX;
  GLint lightVPMatrix = GL_INVALID_INDEX;
  GLint normalMatrix = GL_INVALID_INDEX;
  LightUniforms light;
  GLint camPosition = GL_INVALID_INDEX;
  GLint translucency = GL_INVALID_INDEX;
  GLint SSSWidth = GL_INVALID_INDEX;
};

class SSSApp : public Application {
public:
  SSSApp()
    : m_main(shadersDir)
    , m_blurProgram(shadersDir)
    , m_shadow(shadersDir)
    , m_finalOutput(shadersDir) {}

  bool init(SDL_Window* window, int w, int h) override {
    m_window = window;
    m_viewportW = w;
    m_viewportH = h;

    updateShadowFB();
    updateBlurFB();
    if (!updateMainFBs()) {
      std::cout << "Failed to init main framebuffers" << std::endl;
      return false;
    }

    if (!initPrograms()) {
      std::cout << "Failed to init programs" << std::endl;
      return false;
    }

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
    m_light.position = Vec3f(0.0f, 1.5f, 1.0f);
    m_light.direction = -glm::normalize(m_light.position);
    m_light.updateMatrices();
    return true;
  }

  void cleanup() override {
    m_main.release();
    m_blurProgram.release();
    m_model.cleanGL();
    m_quad.release();
    m_finalOutput.release();
    releaseFBs(true);
  }

  bool update(float deltaT) override {
    updateAvgDeltaT(deltaT);
    return m_keepRunning;
  }

  void beginFrame() override {
    if (m_viewportNeedsUpdate) {
      if (!updateMainFBs()) {
        std::cout << "Failed to update framebuffers" << std::endl;
        m_keepRunning = false;
      }
      m_viewportNeedsUpdate = false;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
  }

  void renderFrame() override {
    shadowPass();
    mainPass();
    blurPass();
    finalOutputPass();
  }

  void endFrame() override { glDisable(GL_DEPTH_TEST); }

  void renderUI() override {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::Button("Close"))
        m_keepRunning = false;

      ImGui::Text("%.2ffps", 1 / m_avgDeltaT);
      ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Config");
    if (ImGui::CollapsingHeader("Subsurface Scattering")) {
      ImGui::SliderFloat("Translucency", &m_translucency, 0.0f, 1.0f);
      ImGui::SliderFloat("Width", &m_SSSWidth, 0.0f, 0.1f);
      ImGui::SliderFloat("lightX", &m_light.position.x, -5.f, 5.f);
      ImGui::SliderFloat("lightY", &m_light.position.y, -5.f, 5.f);
      ImGui::SliderFloat("lightZ", &m_light.position.z, -5.f, 5.f);
    }
    ImGui::End();
  }

private:
  bool initPrograms() {
    return initShadowProgram() && initMainProgram() && initBlurProgram() &&
           initFinalOutputProgram();
  }

  bool initShadowProgram() {
    m_shadow.init();
    if (!m_shadow.addShader("vertex", GL_VERTEX_SHADER, "shadow.vert") ||
        !m_shadow.addShader("fragment", GL_FRAGMENT_SHADER, "shadow.frag")) {
      std::cout << "Could not add shadow shaders" << std::endl;
      return false;
    }
    if (!m_shadow.link())
      return false;

    m_shadowLoc.lightMVP = m_shadow.getUniformLocation("uLightMVP");
    return true;
  }

  bool initMainProgram() {
    m_main.init();
    if (!m_main.addShader("vertex", GL_VERTEX_SHADER, "main.vert") ||
        !m_main.addShader("fragment", GL_FRAGMENT_SHADER, "main.frag")) {
      std::cout << "Could not add main shaders" << std::endl;
      return false;
    }
    if (!m_main.link())
      return false;

    // get uniform locations
    m_loc.modelMatrix = m_main.getUniformLocation("uModelMatrix");
    m_loc.MVPMatrix = m_main.getUniformLocation("uMVPMatrix");
    m_loc.lightVPMatrix = m_main.getUniformLocation("uLightVPMatrix");
    m_loc.normalMatrix = m_main.getUniformLocation("uNormalMatrix");
    m_loc.light.position = m_main.getUniformLocation("uLight.position");
    m_loc.light.farPlane = m_main.getUniformLocation("uLight.farPlane");
    m_loc.camPosition = m_main.getUniformLocation("uCamPosition");
    m_loc.translucency = m_main.getUniformLocation("uTranslucency");
    m_loc.SSSWidth = m_main.getUniformLocation("uSSSWidth");
    return true;
  }

  bool initBlurProgram() {
    m_blurProgram.init();
    if (!m_blurProgram.addShader("vertex", GL_VERTEX_SHADER, "sss-blur.vert") ||
        !m_blurProgram.addShader("fragment", GL_FRAGMENT_SHADER, "sss-blur.frag")) {
      std::cout << "Could not add blur shaders" << std::endl;
      return false;
    }
    if (!m_blurProgram.link())
      return false;

    m_blurLoc.fovy = m_blurProgram.getUniformLocation("uFovy");
    m_blurLoc.sssWidth = m_blurProgram.getUniformLocation("uSssWidth");
    return true;
  }

  bool initFinalOutputProgram() {
    m_finalOutput.init();
    if (!m_finalOutput.addShader("vertex", GL_VERTEX_SHADER, "fb-quad.vert") ||
        !m_finalOutput.addShader("fragment", GL_FRAGMENT_SHADER, "fb-quad-final.frag")) {
      std::cout << "Could not add final output shaders" << std::endl;
      return false;
    }
    return m_finalOutput.link();
  }

  bool updateMainFBs() {
    releaseFBs(false);
    return updateMainFB() && updateBlurFB();
  }

  bool updateBlurFB() {
    glCreateFramebuffers(1, &m_blurFB);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_blurFBTexture);
    glTextureStorage2D(m_blurFBTexture, 1, GL_RGB16F, m_viewportW, m_viewportH);

    glNamedFramebufferTexture(m_blurFB, GL_COLOR_ATTACHMENT0, m_blurFBTexture, 0);
    // glNamedFramebufferDrawBuffer(m_blurFB, GL_NONE);
    return glCheckNamedFramebufferStatus(m_blurFB, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  }

  void updateShadowFB() {
    glCreateFramebuffers(1, &m_shadowFB);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_shadowDepthMap);
    glTextureStorage2D(m_shadowDepthMap, 1, GL_DEPTH_COMPONENT24, shadowRes, shadowRes);

    glNamedFramebufferTexture(m_shadowFB, GL_DEPTH_ATTACHMENT, m_shadowDepthMap, 0);
    glNamedFramebufferDrawBuffer(m_shadowFB, GL_NONE);
  }

  bool updateMainFB() {
    glCreateFramebuffers(1, &m_mainFB);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_mainFBColorTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_mainFBDepthStencilTex);
    glTextureStorage2D(m_mainFBColorTex, 1, GL_RGB16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_mainFBDepthStencilTex, 1, GL_DEPTH24_STENCIL8, m_viewportW, m_viewportH);

    glNamedFramebufferTexture(m_mainFB, GL_COLOR_ATTACHMENT0, m_mainFBColorTex, 0);
    glNamedFramebufferTexture(m_mainFB, GL_DEPTH_STENCIL_ATTACHMENT, m_mainFBDepthStencilTex, 0);
    return glCheckNamedFramebufferStatus(m_mainFB, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  }

  void releaseFBs(bool releaseAll) {
    if (m_mainFB) {
      glDeleteTextures(1, &m_mainFBColorTex);
      glDeleteTextures(1, &m_mainFBDepthStencilTex);
      glDeleteFramebuffers(1, &m_mainFB);
      m_mainFB = 0;
      m_mainFBColorTex = 0;
      m_mainFBDepthStencilTex = 0;
    }
    if (m_blurFB) {
      glDeleteTextures(1, &m_blurFBTexture);
      glDeleteFramebuffers(1, &m_blurFB);
      m_blurFB = 0;
      m_blurFBTexture = 0;
    }
    if (releaseAll) {
      if (m_shadowFB) {
        glDeleteTextures(1, &m_shadowDepthMap);
        glDeleteFramebuffers(1, &m_shadowFB);
        m_shadowFB = 0;
        m_shadowDepthMap = 0;
      }
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

  void shadowPass() const {
    glViewport(0, 0, shadowRes, shadowRes);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFB);

    Mat4f lightMVP = m_light.proj * m_light.view * m_model.transformation();
    m_shadow.setMat4(m_shadowLoc.lightMVP, lightMVP);

    glClear(GL_DEPTH_BUFFER_BIT);
    m_model.render(m_shadow);
  }

  void mainPass() const {
    glViewport(0, 0, m_viewportW, m_viewportH);
    glBindFramebuffer(GL_FRAMEBUFFER, m_mainFB);
    glBindTextureUnit(0, m_shadowDepthMap);

    Mat4f model = m_model.transformation();
    Mat4f mvp = m_cam.projectionMatrix() * m_cam.viewMatrix() * model;
    m_main.setMat4(m_loc.modelMatrix, model);
    m_main.setMat4(m_loc.MVPMatrix, mvp);
    m_main.setMat4(m_loc.lightVPMatrix, m_light.proj * m_light.view);
    m_main.setMat4(m_loc.normalMatrix, glm::transpose(glm::inverse(model)));

    m_main.setVec3(m_loc.light.position, m_light.position);
    m_main.setFloat(m_loc.light.farPlane, m_light.far);

    m_main.setVec3(m_loc.camPosition, m_cam.position());

    m_main.setFloat(m_loc.translucency, m_translucency);
    m_main.setFloat(m_loc.SSSWidth, m_SSSWidth);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_model.render(m_main);

    glBindTextureUnit(0, 0);
  }

  void blurPass() const {
    glViewport(0, 0, m_viewportW, m_viewportH);
    glBindFramebuffer(GL_FRAMEBUFFER, m_blurFB);
    glBindTextureUnit(0, m_mainFBColorTex);
    glBindTextureUnit(1, m_mainFBDepthStencilTex);

    m_blurProgram.setFloat(m_blurLoc.fovy, m_cam.fovy());
    m_blurProgram.setFloat(m_blurLoc.sssWidth, m_SSSWidth);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_quad.render(m_blurProgram);

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
  }

  void finalOutputPass() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTextureUnit(0, m_blurFBTexture);
    glBindTextureUnit(1, m_mainFBDepthStencilTex);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_quad.render(m_finalOutput);

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
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
  ShaderProgram m_main;
  UniformLocations m_loc;
  TriangleMeshModel m_model;

  GLuint m_blurFB = 0;
  GLuint m_blurFBTexture = 0;
  ShaderProgram m_blurProgram;
  BlurUniforms m_blurLoc;

  Light m_light;
  GLuint m_shadowDepthMap = 0;
  GLuint m_shadowFB = 0;
  ShaderProgram m_shadow;
  ShadowUniforms m_shadowLoc;

  // SSS config.
  float m_translucency = 0.85f;
  float m_SSSWidth = 0.05f;

  GLuint m_mainFB = 0;
  GLuint m_mainFBColorTex = 0;
  GLuint m_mainFBDepthStencilTex = 0;
  QuadModel m_quad;
  ShaderProgram m_finalOutput;
};

int main(int argc, char** argv) {
  SSSApp app;
  sss::AppContext ctx(app);
  ctx.start(appName, appW, appH);
  return 0;
}
