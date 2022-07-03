#include "Application.h"
#include "SSSConfig.h"
#include "camera/FreeflyCamera.h"
#include "camera/TrackballCamera.h"
#include "model/MaterialMeshModel.h"
#include "model/QuadMesh.h"
#include "shader/ShaderProgram.h"

#include "utils/Image.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace sss;

const char* appName = "sss";
int appW = 1500;
int appH = 750;

std::string ShaderProgram::s_shadersDir = SSS_ASSET_DIR "/shaders/";
GLsizei shadowRes = 1024;

struct Light {
  float pitch = 0.0f;
  float yaw = 0.0f;
  float distance = 1.0f;
  float near = 0.1f;
  float far = 5.0f;
  float fovy = 45.0f;

  Vec3f position = {};
  Vec3f direction = {};
  Mat4f view = {};
  Mat4f proj = {};

  Light() { update(); }

  void update() {
    updatePosition();

    const Vec3f up(0.0f, 1.0f, 0.0f);
    view = glm::lookAt(position, position + direction, up);
    proj = glm::perspective(glm::radians(fovy), 1.0f, near, far);
  }

private:
  void updatePosition() {
    float pitchRad = glm::radians(pitch);
    float yawRad = glm::radians(yaw);
    Vec3f invDir = glm::normalize(Vec3f(glm::cos(-yawRad) * glm::cos(pitchRad), glm::sin(pitchRad),
                                        glm::sin(-yawRad) * glm::cos(pitchRad)));

    position = -invDir * distance;
    direction = invDir;
  }
};

struct ShadowUniforms {
  GLint lightMVP = GL_INVALID_INDEX;
};

struct LightUniforms {
  GLint position = GL_INVALID_INDEX;
  GLint direction = GL_INVALID_INDEX;
  GLint farPlane = GL_INVALID_INDEX;
};

struct MainUniforms {
  GLint modelMatrix = GL_INVALID_INDEX;
  GLint MVPMatrix = GL_INVALID_INDEX;
  GLint lightVPMatrix = GL_INVALID_INDEX;
  GLint normalMatrix = GL_INVALID_INDEX;
  LightUniforms light;
  GLint camPosition = GL_INVALID_INDEX;
  GLint enableSSS = GL_INVALID_INDEX;
  GLint translucency = GL_INVALID_INDEX;
  GLint SSSWidth = GL_INVALID_INDEX;
  GLint SSSNormalBias = GL_INVALID_INDEX;
};

struct BlurUniforms {
  GLint fovy = GL_INVALID_INDEX;
  GLint sssWidth = GL_INVALID_INDEX;
  GLint numSamples = GL_INVALID_INDEX;
  GLint falloff = GL_INVALID_INDEX;
  GLint strength = GL_INVALID_INDEX;
};

class SSSApp : public Application {
public:
  bool init(SDL_Window* window, int w, int h) override {
    m_window = window;
    m_viewportW = w;
    m_viewportH = h;

    updateShadowFB();
    if (!updateMainFBs()) {
      std::cout << "Failed to init main framebuffers" << std::endl;
      return false;
    }

    if (!initPrograms()) {
      std::cout << "Failed to init programs" << std::endl;
      return false;
    }

    m_cam.setFovy(60.0f);
    m_cam.setLookAt(Vec3f(1.0f, 0.0f, 0.0f));
    m_cam.setPosition(Vec3f(0.0f, 0.0f, 0.0f));
    m_cam.setScreenSize(appW, appH);
    m_cam.setSpeed(0.05f);

    // init models
    m_model.load("james", SSS_ASSET_DIR "/models/james/james_hi.obj");
    m_model.setTransform(glm::scale(m_model.transform(), Vec3f(0.01f)));

    m_quad.init();
    m_light.yaw = 90.0f;
    m_light.position = Vec3f(0.0f, 0.0f, 1.0f);
    m_light.update();

    initMaps();
    return true;
  }

  void cleanup() override {
    m_mainProgram.release();
    m_blurProgram.release();
    m_model.release();
    m_quad.release();
    m_finalOutputProgram.release();
    releaseFBs(true);
  }

  bool update(float deltaT) override {
    updateAvgDeltaT(deltaT);
    return m_keepRunning;
  }

  Texture loadTexture(const std::string& path) {
    const char* path_str = path.c_str();

    Texture texture;
    Image image;
    const std::string fullPath = SSS_ASSET_DIR "/maps/" + path;
    if (image.load(fullPath)) {
      glCreateTextures(GL_TEXTURE_2D, 1, &texture.id);
      texture.path = path_str;
      texture.type = "diffuse";

      GLenum format = GL_INVALID_ENUM;
      GLenum internalFormat = GL_INVALID_ENUM;
      if (image.nbChannels() == 1) {
        format = GL_RED;
        internalFormat = GL_R32F;
      } else if (image.nbChannels() == 2) {
        format = GL_RG;
        internalFormat = GL_RG32F;
      } else if (image.nbChannels() == 3) {
        format = GL_RGB;
        internalFormat = GL_RGB32F;
      } else {
        format = GL_RGBA;
        internalFormat = GL_RGBA32F;
      }

      // Deduce the number of mipmaps.
      int w = image.width();
      int h = image.height();
      int mips = (int)glm::log2((float)glm::max(w, h));
      glTextureStorage2D(texture.id, mips, internalFormat, w, h);
      glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glTextureSubImage2D(texture.id, 0, 0, 0, w, h, format, GL_UNSIGNED_BYTE, image.pixels());
      glGenerateTextureMipmap(texture.id);
    }

    return texture;
  }

  void initMaps() {
    kernelSizeTex = loadTexture("kernelSizeMap.png");
    if (kernelSizeTex.id == GL_INVALID_INDEX) {
      std::cerr << "Error loading texture "
                << "kernelSizeMap.png" << std::endl;
    }
  }

public:
  void beginFrame() override {
    if (m_viewportNeedsUpdate) {
      if (!updateMainFBs()) {
        std::cout << "Failed to update main framebuffers" << std::endl;
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
    if (m_enableBlur)
      blurPass();
    finalOutputPass();
  }

  void endFrame() override { glDisable(GL_DEPTH_TEST); }

public:
  void renderUI() override {
    if (ImGui::BeginMainMenuBar()) {
      ImGui::Text("%.2f fps", 1 / m_avgDeltaT);
      ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Config");
    if (ImGui::CollapsingHeader("Subsurface Scattering")) {
      ImGui::Checkbox("Translucency", &m_enableTranslucency);
      ImGui::Checkbox("Blur", &m_enableBlur);
      ImGui::Checkbox("Stencil test", &m_enableStencilTest);

      ImGui::SliderFloat("Strength", &m_translucency, 0.0f, 1.0f);
      ImGui::SliderFloat("Width", &m_SSSWidth, 0.0001f, 0.1f);
      ImGui::SliderFloat("Normal bias", &m_SSSNormalBias, 0.0f, 1.0f);
      if (ImGui::CollapsingHeader("Kernel")) {
        ImGui::SliderInt("Dim", &m_nSamples, 10, 50);
        ImGui::SliderFloat3("Falloff", glm::value_ptr(m_falloff), 0.0f, 1.0f);
        ImGui::SliderFloat3("Strength", glm::value_ptr(m_strength), 0.0f, 1.0f);
      }

      if (ImGui::CollapsingHeader("Light")) {
        ImGui::SliderFloat("Pitch", &m_light.pitch, -89.0f, 89.0f);
        ImGui::SliderFloat("Yaw", &m_light.yaw, -180.0f, 180.0f);
        ImGui::SliderFloat("Distance", &m_light.distance, 0.01f, 1.5f);
        ImGui::SliderFloat("Far plane", &m_light.far, m_light.near + 0.001f, 7.5f);
        ImGui::SliderFloat("fovy", &m_light.fovy, 5.0f, 90.0f);
        m_light.update();

        ImGui::Checkbox("Show depth map", &m_showDepthMap);
        if (m_showDepthMap)
          ImGui::Image((void*)(size_t)m_shadowDepthTex, {200, 200}, /*uv0=*/{0.0f, 1.0f},
                       /*uv1=*/{1.0f, 0.0f});
      }
    }
    ImGui::End();
  }

private:
  bool initPrograms() {
    return initShadowProgram() && initMainProgram() && initBlurProgram() &&
           initFinalOutputProgram();
  }

  bool initShadowProgram() {
    if (!m_shadowProgram.initVertexFragment("shadow.vert", "shadow.frag")) {
      std::cout << "Failed to init shadow program" << std::endl;
      return false;
    }

    m_shadowUniforms.lightMVP = m_shadowProgram.getUniformLocation("uLightMVP");
    return true;
  }

  bool initMainProgram() {
    if (!m_mainProgram.initVertexFragment("main.vert", "main.frag")) {
      std::cout << "Failed to init main program" << std::endl;
      return false;
    }

    m_mainUniforms.modelMatrix = m_mainProgram.getUniformLocation("uModelMatrix");
    m_mainUniforms.MVPMatrix = m_mainProgram.getUniformLocation("uMVPMatrix");
    m_mainUniforms.lightVPMatrix = m_mainProgram.getUniformLocation("uLightVPMatrix");
    m_mainUniforms.normalMatrix = m_mainProgram.getUniformLocation("uNormalMatrix");
    m_mainUniforms.light.position = m_mainProgram.getUniformLocation("uLight.position");
    m_mainUniforms.light.direction = m_mainProgram.getUniformLocation("uLight.direction");
    m_mainUniforms.light.farPlane = m_mainProgram.getUniformLocation("uLight.farPlane");
    m_mainUniforms.camPosition = m_mainProgram.getUniformLocation("uCamPosition");
    m_mainUniforms.enableSSS = m_mainProgram.getUniformLocation("uEnableTranslucency");
    m_mainUniforms.translucency = m_mainProgram.getUniformLocation("uTranslucency");
    m_mainUniforms.SSSWidth = m_mainProgram.getUniformLocation("uSSSWidth");
    m_mainUniforms.SSSNormalBias = m_mainProgram.getUniformLocation("uSSSNormalBias");
    return true;
  }

  bool initBlurProgram() {
    if (!m_blurProgram.initVertexFragment("sss-blur.vert", "sss-blur.frag")) {
      std::cout << "Failed to init blur program" << std::endl;
      return false;
    }

    m_blurUniforms.fovy = m_blurProgram.getUniformLocation("uFovy");
    m_blurUniforms.sssWidth = m_blurProgram.getUniformLocation("uSSSWidth");
    m_blurUniforms.numSamples = m_blurProgram.getUniformLocation("numSamples");
    m_blurUniforms.falloff = m_blurProgram.getUniformLocation("falloff");
    m_blurUniforms.strength = m_blurProgram.getUniformLocation("strength");
    return true;
  }

  bool initFinalOutputProgram() {
    return m_finalOutputProgram.initVertexFragment("fb-quad.vert", "fb-quad-final.frag");
  }

private:
  bool updateMainFBs() {
    releaseFBs(false);
    return updateMainFB() && updateBlurFB();
  }

  bool updateBlurFB() {
    glCreateFramebuffers(1, &m_blurFB);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_blurFBColorTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_blurFBStencilTex);
    glTextureStorage2D(m_blurFBColorTex, 1, GL_RGB16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_blurFBStencilTex, 1, GL_STENCIL_INDEX8, m_viewportW, m_viewportH);

    glNamedFramebufferTexture(m_blurFB, GL_COLOR_ATTACHMENT0, m_blurFBColorTex, 0);
    glNamedFramebufferTexture(m_blurFB, GL_STENCIL_ATTACHMENT, m_blurFBStencilTex, 0);
    return glCheckNamedFramebufferStatus(m_blurFB, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  }

  void updateShadowFB() {
    glCreateFramebuffers(1, &m_shadowFB);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_shadowDepthTex);
    glTextureStorage2D(m_shadowDepthTex, 1, GL_DEPTH_COMPONENT24, shadowRes, shadowRes);

    // Read the texture as grayscale for visualizing, (the shader still only uses the red
    // component).
    glTextureParameteri(m_shadowDepthTex, GL_TEXTURE_SWIZZLE_R, GL_RED);
    glTextureParameteri(m_shadowDepthTex, GL_TEXTURE_SWIZZLE_G, GL_RED);
    glTextureParameteri(m_shadowDepthTex, GL_TEXTURE_SWIZZLE_B, GL_RED);

    glNamedFramebufferTexture(m_shadowFB, GL_DEPTH_ATTACHMENT, m_shadowDepthTex, 0);
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

  void releaseFBs(bool releaseFixedSize) {
    if (m_blurFB) {
      glDeleteTextures(1, &m_blurFBColorTex);
      glDeleteTextures(1, &m_blurFBStencilTex);
      glDeleteFramebuffers(1, &m_blurFB);
      m_blurFB = 0;
      m_blurFBColorTex = 0;
      m_blurFBStencilTex = 0;
    }
    if (m_mainFB) {
      glDeleteTextures(1, &m_mainFBColorTex);
      glDeleteTextures(1, &m_mainFBDepthStencilTex);
      glDeleteFramebuffers(1, &m_mainFB);
      m_mainFB = 0;
      m_mainFBColorTex = 0;
      m_mainFBDepthStencilTex = 0;
    }
    if (releaseFixedSize) {
      if (m_shadowFB) {
        glDeleteTextures(1, &m_shadowDepthTex);
        glDeleteFramebuffers(1, &m_shadowFB);
        m_shadowFB = 0;
        m_shadowDepthTex = 0;
      }
    }
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

private: // Render passes.
  void shadowPass() const {
    glViewport(0, 0, shadowRes, shadowRes);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFB);

    Mat4f lightMVP = m_light.proj * m_light.view * m_model.transform();
    m_shadowProgram.setMat4(m_shadowUniforms.lightMVP, lightMVP);

    glClear(GL_DEPTH_BUFFER_BIT);
    m_model.render(m_shadowProgram);
  }

  void mainPass() const {
    glViewport(0, 0, m_viewportW, m_viewportH);
    glBindFramebuffer(GL_FRAMEBUFFER, m_mainFB);
    glBindTextureUnit(0, m_shadowDepthTex);

    Mat4f model = m_model.transform();
    Mat4f mvp = m_cam.projectionMatrix() * m_cam.viewMatrix() * model;
    m_mainProgram.setMat4(m_mainUniforms.modelMatrix, model);
    m_mainProgram.setMat4(m_mainUniforms.MVPMatrix, mvp);
    m_mainProgram.setMat4(m_mainUniforms.lightVPMatrix, m_light.proj * m_light.view);
    m_mainProgram.setMat4(m_mainUniforms.normalMatrix, glm::transpose(glm::inverse(model)));

    m_mainProgram.setVec3(m_mainUniforms.light.position, m_light.position);
    m_mainProgram.setVec3(m_mainUniforms.light.direction, m_light.direction);
    m_mainProgram.setFloat(m_mainUniforms.light.farPlane, m_light.far);

    m_mainProgram.setVec3(m_mainUniforms.camPosition, m_cam.position());

    m_mainProgram.setBool(m_mainUniforms.enableSSS, m_enableTranslucency);
    m_mainProgram.setFloat(m_mainUniforms.translucency, m_translucency);
    m_mainProgram.setFloat(m_mainUniforms.SSSWidth, m_SSSWidth);
    m_mainProgram.setFloat(m_mainUniforms.SSSNormalBias, m_SSSNormalBias);

    if (m_enableStencilTest) {
      glEnable(GL_STENCIL_TEST);
      glStencilMask(0xFF);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    if (m_enableStencilTest) {
      glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
      glStencilFunc(GL_ALWAYS, 1, 0xFF);
    }

    m_model.render(m_mainProgram);

    if (m_enableStencilTest) {
      glStencilMask(0x00);
      glDisable(GL_STENCIL_TEST);
    }
    glBindTextureUnit(0, 0);
  }

  void blurPass() const {
    if (m_enableStencilTest) {
      // Copy the main stencil buffer to the blur FB.
      glBlitNamedFramebuffer(m_mainFB, m_blurFB, 0, 0, m_viewportW, m_viewportH, 0, 0, m_viewportW,
                             m_viewportH, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    }

    glViewport(0, 0, m_viewportW, m_viewportH);
    glBindFramebuffer(GL_FRAMEBUFFER, m_blurFB);
    glBindTextureUnit(0, m_mainFBColorTex);
    glBindTextureUnit(1, m_mainFBDepthStencilTex);
    glBindTextureUnit(2, kernelSizeTex.id);

    m_blurProgram.setFloat(m_blurUniforms.fovy, m_cam.fovy());
    m_blurProgram.setFloat(m_blurUniforms.sssWidth, m_SSSWidth);
    m_blurProgram.setInt(m_blurUniforms.numSamples, m_nSamples);
    m_blurProgram.setVec3(m_blurUniforms.falloff, m_falloff);
    m_blurProgram.setVec3(m_blurUniforms.strength, m_strength);

    glClear(GL_COLOR_BUFFER_BIT);
    if (m_enableStencilTest) {
      glEnable(GL_STENCIL_TEST);
      glStencilMask(0x00);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
      glStencilFunc(GL_EQUAL, 1, 0xFF);
    }

    m_quad.render(m_blurProgram);

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
    glBindTextureUnit(2, 0);

    if (m_enableStencilTest)
      glDisable(GL_STENCIL_TEST);
  }

  void finalOutputPass() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (m_enableBlur)
      glBindTextureUnit(0, m_blurFBColorTex);
    else
      glBindTextureUnit(0, m_mainFBColorTex);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_quad.render(m_finalOutputProgram);

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
  TrackballCamera m_ffCam;
  ShaderProgram m_mainProgram;
  MainUniforms m_mainUniforms;
  MaterialMeshModel m_model;

  GLuint m_blurFB = 0;
  GLuint m_blurFBColorTex = 0;
  GLuint m_blurFBStencilTex = 0;
  ShaderProgram m_blurProgram;
  BlurUniforms m_blurUniforms;

  Light m_light;
  GLuint m_shadowDepthTex = 0;
  GLuint m_shadowFB = 0;
  ShaderProgram m_shadowProgram;
  ShadowUniforms m_shadowUniforms;

  // SSS config.
  bool m_enableTranslucency = true;
  bool m_enableBlur = true;
  bool m_enableStencilTest = true;
  bool m_showDepthMap = true;
  float m_translucency = 0.75f;
  float m_SSSWidth = 0.015f;
  float m_SSSNormalBias = 0.3f;
  int m_nSamples = 20;
  Vec3f m_falloff = Vec3f(1.0f, 0.37f, 0.3f);
  Vec3f m_strength = Vec3f(0.48f, 0.41f, 0.28f);

  GLuint m_mainFB = 0;
  GLuint m_mainFBColorTex = 0;
  GLuint m_mainFBDepthStencilTex = 0;
  QuadMesh m_quad;
  ShaderProgram m_finalOutputProgram;

  Texture kernelSizeTex;
};

int main(int argc, char** argv) {
  SSSApp app;
  sss::AppContext ctx(app);
  ctx.start(appName, appW, appH);
  return 0;
}
