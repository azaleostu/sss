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

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

using namespace sss;

const char* appName = "sss";

#if 0
int appW = 1500;
int appH = 750;
#else
int appW = 1600;
int appH = 900;
#endif

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

struct GBufUniforms {
  GLint modelMatrix = GL_INVALID_INDEX;
  GLint MVPMatrix = GL_INVALID_INDEX;
  GLint normalMatrix = GL_INVALID_INDEX;
  LightUniforms light;
};

struct MainUniforms {
  GLint lightVPMatrix = GL_INVALID_INDEX;
  GLint camPosition = GL_INVALID_INDEX;
  LightUniforms light;
  GLint enableTranslucency = GL_INVALID_INDEX;
  GLint enableBlur = GL_INVALID_INDEX;
  GLint translucency = GL_INVALID_INDEX;
  GLint SSSWeight = GL_INVALID_INDEX;
  GLint SSSWidth = GL_INVALID_INDEX;
  GLint SSSNormalBias = GL_INVALID_INDEX;
};

struct BlurUniforms {
  GLint fovy = GL_INVALID_INDEX;
  GLint sssWidth = GL_INVALID_INDEX;
  GLint numSamples = GL_INVALID_INDEX;
  GLint falloff = GL_INVALID_INDEX;
  GLint strength = GL_INVALID_INDEX;
  GLint photonPathLength = GL_INVALID_INDEX;
};

// input melanin/hemoglobin concentration [0:1]
// outputs
Vec2f getSkinLookupUv(float melanin, float hemoglobin) {
  melanin = glm::clamp(melanin, 0.f, 0.5f) / 0.5f;
  hemoglobin = glm::clamp(hemoglobin, 0.f, 0.32f) / 0.32f;
  return {cbrtf(melanin), cbrtf(hemoglobin)};
}

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

    if (!initMaps()) {
      std::cout << "Failed to init maps" << std::endl;
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
    return true;
  }

  void cleanup() override {
    m_shadowProgram.release();
    m_GBufProgram.release();
    m_mainProgram.release();
    m_blurProgram.release();
    m_finalOutputProgram.release();
    m_model.release();
    m_quad.release();
    m_kernelSizeTex.release();
    m_modelSkinParamMap.release();
    releaseFBs(true);
  }

  bool update(float deltaT) override {
    updateAvgDeltaT(deltaT);
    return m_keepRunning;
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

    GBufPass();
    if (m_enableBlur)
      blurPass();
    mainPass();

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

      if (m_enableBlur || m_enableTranslucency) {
        ImGui::SliderFloat("Strength", &m_translucency, 0.0f, 1.0f);
        ImGui::SliderFloat("Width", &m_SSSWidth, 0.0001f, 0.1f);
      }

      if (m_enableTranslucency)
        ImGui::SliderFloat("Normal bias", &m_SSSNormalBias, 0.0f, 1.0f);

      if (m_enableBlur) {
        ImGui::SliderFloat("Weight", &m_SSSWeight, 0.0f, 1.0f);
        if (ImGui::CollapsingHeader("Kernel")) {
          ImGui::SliderInt("Dim", &m_nSamples, 10, 50);
          ImGui::SliderFloat3("Falloff", glm::value_ptr(m_falloff), 0.0f, 1.0f);
          ImGui::SliderFloat3("Strength", glm::value_ptr(m_strength), 0.0f, 1.0f);
          ImGui::SliderFloat("Photon Path Length", &m_photonPathLength, 1.0f, 20.0f);
        }
      }

      if (ImGui::CollapsingHeader("Light")) {
        ImGui::SliderFloat("Pitch", &m_light.pitch, -89.0f, 89.0f);
        ImGui::SliderFloat("Yaw", &m_light.yaw, -180.0f, 180.0f);
        ImGui::SliderFloat("Distance", &m_light.distance, 0.01f, 1.5f);
        ImGui::SliderFloat("Far plane", &m_light.far, m_light.near + 0.001f, 7.5f);
        ImGui::SliderFloat("fovy", &m_light.fovy, 5.0f, 90.0f);
        m_light.update();

        ImGui::Image((void*)(size_t)m_shadowDepthTex, {200, 200}, /*uv0=*/{0.0f, 1.0f},
                     /*uv1=*/{1.0f, 0.0f});
      }
    }

    renderGBufVisualizerUI();
    ImGui::End();
  }

  void renderGBufVisualizerUI() {
    GLuint textures[] = {m_GBufPosTex, m_GBufUVTex, m_GBufNormalTex, m_GBufIrradianceTex};
    constexpr unsigned int NumTextures = ARRAY_LENGTH(textures);

    if (ImGui::CollapsingHeader("G-Buffer")) {
      if (ImGui::BeginTable("GBuf-vis-header", 3, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableNextColumn();
        if (ImGui::Button("Prev")) {
          if (m_GBufVisTextureIndex == 0)
            m_GBufVisTextureIndex = NumTextures - 1;
          else
            m_GBufVisTextureIndex -= 1;
        }

        ImGui::TableNextColumn();
        if (ImGui::Button("Next"))
          m_GBufVisTextureIndex = (m_GBufVisTextureIndex + 1) % NumTextures;

        ImGui::TableNextColumn();
        ImGui::Text("%d/%d", m_GBufVisTextureIndex + 1, NumTextures);
        ImGui::EndTable();
      }

      const float scale = 2.0f;
      ImGui::Image((void*)(size_t)textures[m_GBufVisTextureIndex], {160 * scale, 90 * scale},
                   /*uv0=*/{0.0f, 1.0f}, /*uv1=*/{1.0f, 0.0f});
    }
  }

private:
  bool initPrograms() {
    return initShadowProgram() && initGBufProgram() && initMainProgram() && initBlurProgram() &&
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

  bool initGBufProgram() {
    if (!m_GBufProgram.initVertexFragment("g-buffer.vert", "g-buffer.frag")) {
      std::cout << "Failed to init GBuf program" << std::endl;
      return false;
    }

    m_GBufUniforms.modelMatrix = m_GBufProgram.getUniformLocation("uModelMatrix");
    m_GBufUniforms.MVPMatrix = m_GBufProgram.getUniformLocation("uMVPMatrix");
    m_GBufUniforms.normalMatrix = m_GBufProgram.getUniformLocation("uNormalMatrix");
    m_GBufUniforms.light.position = m_GBufProgram.getUniformLocation("uLight.position");
    m_GBufUniforms.light.direction = m_GBufProgram.getUniformLocation("uLight.direction");
    return true;
  }

  bool initMainProgram() {
    if (!m_mainProgram.initVertexFragment("fb-quad.vert", "g-buffer-main.frag")) {
      std::cout << "Failed to init main program" << std::endl;
      return false;
    }

    m_mainUniforms.lightVPMatrix = m_mainProgram.getUniformLocation("uLightVPMatrix");
    m_mainUniforms.camPosition = m_mainProgram.getUniformLocation("uCamPosition");
    m_mainUniforms.light.position = m_mainProgram.getUniformLocation("uLight.position");
    m_mainUniforms.light.direction = m_mainProgram.getUniformLocation("uLight.direction");
    m_mainUniforms.light.farPlane = m_mainProgram.getUniformLocation("uLight.farPlane");
    m_mainUniforms.enableTranslucency = m_mainProgram.getUniformLocation("uEnableTranslucency");
    m_mainUniforms.enableBlur = m_mainProgram.getUniformLocation("uEnableBlur");
    m_mainUniforms.translucency = m_mainProgram.getUniformLocation("uTranslucency");
    m_mainUniforms.SSSWeight = m_mainProgram.getUniformLocation("uSSSWeight");
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
    m_blurUniforms.numSamples = m_blurProgram.getUniformLocation("uNumSamples");
    m_blurUniforms.falloff = m_blurProgram.getUniformLocation("uFalloff");
    m_blurUniforms.strength = m_blurProgram.getUniformLocation("uStrength");
    m_blurUniforms.photonPathLength = m_blurProgram.getUniformLocation("uPhotonPathLength");
    return true;
  }

  bool initFinalOutputProgram() {
    return m_finalOutputProgram.initVertexFragment("fb-quad.vert", "fb-quad-final.frag");
  }

private:
  static Texture loadTexture(const std::string& path) {
    const char* path_str = path.c_str();

    Texture texture;
    Image image;
    const std::string fullPath = SSS_ASSET_DIR "/" + path;
    if (!image.load(fullPath)) {
      texture.id = GL_INVALID_INDEX;
      return texture;
    }

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

    return texture;
  }

  bool initMaps() {
    m_kernelSizeTex = loadTexture("maps/kernelSizeMap.png");
    m_modelSkinParamMap = loadTexture("models/james/textures/james_skin_params.png");
    return m_kernelSizeTex.isValid() && m_modelSkinParamMap.isValid();
  }

private:
  bool updateMainFBs() {
    releaseFBs(false);
    return updateGBufFB() && updateMainFB() && updateBlurFB();
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

  bool updateGBufFB() {
    glCreateFramebuffers(1, &m_GBufFB);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_GBufPosTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_GBufUVTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_GBufNormalTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_GBufIrradianceTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_GBufDepthStencilTex);

    glTextureStorage2D(m_GBufPosTex, 1, GL_RGB16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_GBufUVTex, 1, GL_RG16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_GBufNormalTex, 1, GL_RGB16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_GBufIrradianceTex, 1, GL_RGB16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_GBufDepthStencilTex, 1, GL_DEPTH24_STENCIL8, m_viewportW, m_viewportH);

    glNamedFramebufferTexture(m_GBufFB, GL_COLOR_ATTACHMENT0, m_GBufPosTex, 0);
    glNamedFramebufferTexture(m_GBufFB, GL_COLOR_ATTACHMENT1, m_GBufUVTex, 0);
    glNamedFramebufferTexture(m_GBufFB, GL_COLOR_ATTACHMENT2, m_GBufNormalTex, 0);
    glNamedFramebufferTexture(m_GBufFB, GL_COLOR_ATTACHMENT3, m_GBufIrradianceTex, 0);
    glNamedFramebufferTexture(m_GBufFB, GL_DEPTH_STENCIL_ATTACHMENT, m_GBufDepthStencilTex, 0);

    return glCheckNamedFramebufferStatus(m_GBufFB, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  }

  bool updateMainFB() {
    glCreateFramebuffers(1, &m_mainFB);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_mainFBColorTex);
    glTextureStorage2D(m_mainFBColorTex, 1, GL_RGB16F, m_viewportW, m_viewportH);

    glNamedFramebufferTexture(m_mainFB, GL_COLOR_ATTACHMENT0, m_mainFBColorTex, 0);
    return glCheckNamedFramebufferStatus(m_mainFB, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
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
      glDeleteFramebuffers(1, &m_mainFB);
      m_mainFB = 0;
      m_mainFBColorTex = 0;
    }

    if (m_GBufFB) {
      glDeleteTextures(1, &m_GBufPosTex);
      glDeleteTextures(1, &m_GBufUVTex);
      glDeleteTextures(1, &m_GBufNormalTex);
      glDeleteTextures(1, &m_GBufIrradianceTex);
      glDeleteTextures(1, &m_GBufDepthStencilTex);
      glDeleteFramebuffers(1, &m_GBufFB);
      m_GBufFB = 0;
      m_GBufPosTex = 0;
      m_GBufUVTex = 0;
      m_GBufNormalTex = 0;
      m_GBufIrradianceTex = 0;
      m_GBufDepthStencilTex = 0;
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

private:
  void shadowPass() const {
    glViewport(0, 0, shadowRes, shadowRes);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFB);

    Mat4f lightMVP = m_light.proj * m_light.view * m_model.transform();
    m_shadowProgram.setMat4(m_shadowUniforms.lightMVP, lightMVP);

    glClear(GL_DEPTH_BUFFER_BIT);
    m_model.render(m_shadowProgram);
  }

  void GBufPass() const {
    glViewport(0, 0, m_viewportW, m_viewportH);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glBindFramebuffer(GL_FRAMEBUFFER, m_GBufFB);

    unsigned int buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
                              GL_COLOR_ATTACHMENT3};
    glDrawBuffers(ARRAY_LENGTH(buffers), buffers);

    Mat4f model = m_model.transform();
    Mat4f mvp = m_cam.projectionMatrix() * m_cam.viewMatrix() * model;
    m_GBufProgram.setMat4(m_GBufUniforms.modelMatrix, model);
    m_GBufProgram.setMat4(m_GBufUniforms.MVPMatrix, mvp);
    m_GBufProgram.setMat4(m_GBufUniforms.normalMatrix, glm::transpose(glm::inverse(model)));

    m_GBufProgram.setVec3(m_GBufUniforms.light.position, m_light.position);
    m_GBufProgram.setVec3(m_GBufUniforms.light.direction, m_light.direction);

    if (m_enableStencilTest) {
      glEnable(GL_STENCIL_TEST);
      glStencilMask(0xFF);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    if (m_enableStencilTest) {
      glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
      glStencilFunc(GL_ALWAYS, 1, 0xFF);
    }

    glBindTextureUnit(3, m_modelSkinParamMap.id);
    m_model.renderForGBuf(m_GBufProgram);
    glBindTextureUnit(3, 0);

    if (m_enableStencilTest) {
      glStencilMask(0x00);
      glDisable(GL_STENCIL_TEST);
    }

    unsigned int buffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &buffer);
  }

  void mainPass() const {
    glViewport(0, 0, m_viewportW, m_viewportH);
    glBindFramebuffer(GL_FRAMEBUFFER, m_mainFB);

    glBindTextureUnit(0, m_shadowDepthTex);

    glBindTextureUnit(1, m_GBufPosTex);
    glBindTextureUnit(2, m_GBufUVTex);
    glBindTextureUnit(3, m_GBufNormalTex);
    glBindTextureUnit(4, m_GBufIrradianceTex);
    glBindTextureUnit(5, m_blurFBColorTex);

    m_model.bindMeshAlbedo(0, 6);

    m_mainProgram.setMat4(m_mainUniforms.lightVPMatrix, m_light.proj * m_light.view);
    m_mainProgram.setVec3(m_mainUniforms.camPosition, m_cam.position());
    m_mainProgram.setVec3(m_mainUniforms.light.position, m_light.position);
    m_mainProgram.setVec3(m_mainUniforms.light.direction, m_light.direction);
    m_mainProgram.setFloat(m_mainUniforms.light.farPlane, m_light.far);

    m_mainProgram.setBool(m_mainUniforms.enableTranslucency, m_enableTranslucency);
    m_mainProgram.setBool(m_mainUniforms.enableBlur, m_enableBlur);
    m_mainProgram.setFloat(m_mainUniforms.translucency, m_translucency);
    m_mainProgram.setFloat(m_mainUniforms.SSSWeight, m_SSSWeight);
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

    m_quad.render(m_mainProgram);

    if (m_enableStencilTest) {
      glStencilMask(0x00);
      glDisable(GL_STENCIL_TEST);
    }

    glBindTextureUnit(0, 0);

    glBindTextureUnit(1, 0);
    glBindTextureUnit(2, 0);
    glBindTextureUnit(3, 0);
    glBindTextureUnit(4, 0);
    glBindTextureUnit(5, 0);

    glBindTextureUnit(6, 0);
  }

  void blurPass() const {
    if (m_enableStencilTest) {
      // Copy the GBuf stencil buffer to the blur FB.
      glBlitNamedFramebuffer(m_GBufFB, m_blurFB, 0, 0, m_viewportW, m_viewportH, 0, 0, m_viewportW,
                             m_viewportH, GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    }

    glViewport(0, 0, m_viewportW, m_viewportH);
    glBindFramebuffer(GL_FRAMEBUFFER, m_blurFB);

    glBindTextureUnit(0, m_GBufIrradianceTex);
    glBindTextureUnit(1, m_GBufDepthStencilTex);
    glBindTextureUnit(2, m_kernelSizeTex.id);
    glBindTextureUnit(3, m_GBufUVTex);

    m_blurProgram.setFloat(m_blurUniforms.fovy, m_cam.fovy());
    m_blurProgram.setFloat(m_blurUniforms.sssWidth, m_SSSWidth);
    m_blurProgram.setInt(m_blurUniforms.numSamples, m_nSamples);
    m_blurProgram.setVec3(m_blurUniforms.falloff, m_falloff);
    m_blurProgram.setVec3(m_blurUniforms.strength, m_strength);
    m_blurProgram.setFloat(m_blurUniforms.photonPathLength, m_photonPathLength);

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
    glBindTextureUnit(3, 0);

    if (m_enableStencilTest)
      glDisable(GL_STENCIL_TEST);
  }

  void finalOutputPass() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
  Texture m_modelSkinParamMap;

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
  float m_translucency = 0.75f;
  float m_SSSWeight = 0.5f;
  float m_SSSWidth = 0.015f;
  float m_SSSNormalBias = 0.3f;
  int m_nSamples = 20;
  Vec3f m_falloff = Vec3f(1.0f, 0.37f, 0.3f);
  Vec3f m_strength = Vec3f(0.48f, 0.41f, 0.28f);
  float m_photonPathLength = 2.f;

  GLuint m_mainFB = 0;
  GLuint m_mainFBColorTex = 0;
  QuadMesh m_quad;
  ShaderProgram m_finalOutputProgram;

  Texture m_kernelSizeTex;

  GLuint m_skinParamTex = 0;
  GLuint m_GBufFB = 0;
  GLuint m_GBufPosTex = 0;
  GLuint m_GBufUVTex = 0;
  GLuint m_GBufNormalTex = 0;
  GLuint m_GBufIrradianceTex = 0;
  GLuint m_GBufDepthStencilTex = 0;
  GBufUniforms m_GBufUniforms;
  ShaderProgram m_GBufProgram;

  // GBuffer visualizer.
  unsigned int m_GBufVisTextureIndex = 0;
};

int main(int argc, char** argv) {
  SSSApp app;
  sss::AppContext ctx(app);
  ctx.start(appName, appW, appH);
  return 0;
}
