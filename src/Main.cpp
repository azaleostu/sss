#include "Application.h"
#include "SSSConfig.h"
#include "camera/FreeflyCamera.h"
#include "camera/TrackballCamera.h"
#include "model/CubeMesh.h"
#include "model/MaterialMeshModel.h"
#include "model/QuadMesh.h"
#include "shader/ShaderProgram.h"
#include "utils/Image.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

using namespace sss;

constexpr const char* AppName = "sss";

#if 0
constexpr int AppW = 1500;
constexpr int AppH = 750;
#else
constexpr int AppW = 1600;
constexpr int AppH = 900;
#endif

std::string ShaderProgram::s_shadersDir = SSS_ASSET_DIR "/shaders/";
const std::string EnvColorMapPath = SSS_ASSET_DIR "/maps/env/Siggraph2007_UpperFloor_REF.hdr";
const std::string EnvIrradianceMapPath = SSS_ASSET_DIR "/maps/env/Siggraph2007_UpperFloor_Env.hdr";

constexpr GLsizei EnvColorSize = 2048;
constexpr GLsizei EnvIrradianceSize = 128;
constexpr GLsizei ShadowMapSize = 1024;

struct Light {
  float pitch = 0.0f;
  float yaw = 0.0f;
  float distance = 1.0f;
  float near = 0.1f;
  float far = 5.0f;
  float fovy = 45.0f;

  Vec3f color = Vec3f(1.0f);
  float intensity = 1.0f;
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

struct SkyBoxUniforms {
  GLint proj = GL_INVALID_INDEX;
  GLint view = GL_INVALID_INDEX;
};

struct LightUniforms {
  GLint position = GL_INVALID_INDEX;
  GLint direction = GL_INVALID_INDEX;
  GLint color = GL_INVALID_INDEX;
  GLint intensity = GL_INVALID_INDEX;
  GLint VPMatrix = GL_INVALID_INDEX;
};

struct GBufUniforms {
  GLint modelMatrix = GL_INVALID_INDEX;
  GLint MVPMatrix = GL_INVALID_INDEX;
  GLint normalMatrix = GL_INVALID_INDEX;
  LightUniforms light;
  GLint gammaCorrect = GL_INVALID_INDEX;
  GLint useDynamicSkinColor = GL_INVALID_INDEX;
  GLint useEnvIrradiance = GL_INVALID_INDEX;
  GLint B = GL_INVALID_INDEX;
  GLint S = GL_INVALID_INDEX;
  GLint F = GL_INVALID_INDEX;
  GLint W = GL_INVALID_INDEX;
  GLint M = GL_INVALID_INDEX;
};

struct MainUniforms {
  LightUniforms light;
  GLint enableTransmittance = GL_INVALID_INDEX;
  GLint enableBlur = GL_INVALID_INDEX;
  GLint transmittanceStrength = GL_INVALID_INDEX;
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

struct FinalOutputUniforms {
  GLint gammaCorrect = GL_INVALID_INDEX;
  GLint exposure = GL_INVALID_INDEX;
};

class SSSApp : public Application {
public:
  bool init(SDL_Window* window, int w, int h) override {
    m_window = window;
    m_viewportW = w;
    m_viewportH = h;

    m_cube.init();
    m_quad.init();

    initShadowFB();
    initEnvFB();
    if (!updateMainFBs()) {
      std::cout << "Failed to init main framebuffers" << std::endl;
      return false;
    }

    if (!initMaps()) {
      std::cout << "Failed to init maps" << std::endl;
      return false;
    }

    if (!renderEnvCubeMaps()) {
      std::cout << "Failed to render env cube maps" << std::endl;
      return false;
    }

    if (!initPrograms()) {
      std::cout << "Failed to init programs" << std::endl;
      return false;
    }

    m_cam.setFovy(60.0f);
    m_cam.setLookAt(Vec3f(1.0f, 0.0f, 0.0f));
    m_cam.setPosition(Vec3f(0.0f, 0.0f, 0.0f));
    m_cam.setScreenSize(AppW, AppH);
    m_cam.setSpeed(0.05f);

    m_model.load("james", SSS_ASSET_DIR "/models/james/james_hi.obj");
    m_model.setTransform(glm::scale(m_model.transform(), Vec3f(0.01f)));

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
    m_paramTex.release();
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
      ImGui::Checkbox("Show config", &m_showConfig);
      ImGui::Separator();
      ImGui::Text("%.2f fps", 1 / m_avgDeltaT);
      ImGui::EndMainMenuBar();
    }

    if (!m_showConfig)
      return;

    ImGui::Begin("Config");
    if (ImGui::CollapsingHeader("Image")) {
      ImGui::Checkbox("Gamma-correct", &m_gammaCorrect);
      ImGui::Checkbox("Show sky-box", &m_showSkyBox);
      ImGui::Checkbox("Use env irradiance", &m_useEnvIrradiance);
      ImGui::SliderFloat("Exposure", &m_exposure, 0.0f, 10.0f);
    }

    if (ImGui::CollapsingHeader("Camera")) {
      if (&m_cam == (BaseCamera*)&m_trackballCam) {
        ImGui::SliderFloat("Zoom speed", &m_trackballCam.zoomSpeed, 0.001f, 0.25f);
        ImGui::SliderFloat("Move speed", &m_trackballCam.moveSpeed, 0.1f, 1.0f);
      }
    }

    if (ImGui::CollapsingHeader("Skin")) {
      ImGui::Checkbox("Dynamic color", &m_useDynamicSkinColor);
      ImGui::SliderFloat("Blood (B)", &m_B, 0.0f, 100.f);
      ImGui::SliderFloat("Oxygenation (S)", &m_S, 0.0f, 100.f);
      ImGui::SliderFloat("Fat (F)", &m_F, 0.0f, 100.f);
      ImGui::SliderFloat("Water (W)", &m_W, 0.0f, 100.f);
      ImGui::SliderFloat("Melanosomes (M)", &m_M, 0.0f, 100.f);
    }

    if (ImGui::CollapsingHeader("SSS")) {
      ImGui::Checkbox("Transmittance", &m_enableTransmittance);
      ImGui::Checkbox("Blur", &m_enableBlur);

      if (m_enableBlur || m_enableTransmittance)
        ImGui::SliderFloat("Effect width", &m_SSSWidth, 0.001f, 0.1f);

      if (m_enableTransmittance) {
        ImGui::Text("Transmittance");
        ImGui::SliderFloat("Strength", &m_transmittanceStrength, 0.0f, 1.0f);
        ImGui::SliderFloat("Normal bias", &m_SSSNormalBias, 0.0f, 1.0f);
      }

      if (m_enableBlur) {
        ImGui::Text("Blur");
        ImGui::SliderFloat("Weight", &m_SSSWeight, 0.0f, 1.0f);
        ImGui::SliderInt("Kernel size", &m_nSamples, 10, 50);
        ImGui::SliderFloat3("Falloff", glm::value_ptr(m_falloff), 0.0f, 1.0f);
        ImGui::SliderFloat3("Strength", glm::value_ptr(m_strength), 0.0f, 1.0f);
        ImGui::SliderFloat("Path Length", &m_photonPathLength, 1.0f, 20.0f);
      }
    }

    if (ImGui::CollapsingHeader("Light")) {
      ImGui::SliderFloat("Pitch", &m_light.pitch, -89.0f, 89.0f);
      ImGui::SliderFloat("Yaw", &m_light.yaw, -180.0f, 180.0f);
      ImGui::SliderFloat("Distance", &m_light.distance, 0.01f, 1.5f);
      ImGui::SliderFloat("Far plane", &m_light.far, m_light.near + 0.001f, 7.5f);
      ImGui::SliderFloat("Fovy", &m_light.fovy, 5.0f, 90.0f);
      ImGui::ColorPicker3("Color", glm::value_ptr(m_light.color));
      ImGui::SliderFloat("Intensity", &m_light.intensity, 0.0f, 20.0f);
      m_light.update();

      ImGui::Image((void*)(size_t)m_shadowDepthTex, {200, 200}, /*uv0=*/{0.0f, 1.0f},
                   /*uv1=*/{1.0f, 0.0f});
    }

    renderGBufVisualizerUI();
    ImGui::End();
  }

  void renderGBufVisualizerUI() {
    GLuint textures[] = {m_GBufPosTex, m_GBufUVTex, m_GBufNormalTex, m_GBufAlbedoTex,
                         m_GBufIrradianceTex};
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
    return initShadowProgram() && initSkyBoxProgram() && initGBufProgram() && initMainProgram() &&
           initBlurProgram() && initFinalOutputProgram();
  }

  bool initShadowProgram() {
    if (!m_shadowProgram.initVertexFragment("shadow.vert", "shadow.frag")) {
      std::cout << "Failed to init shadow program" << std::endl;
      return false;
    }

    m_shadowUniforms.lightMVP = m_shadowProgram.getUniformLocation("uLightMVP");
    return true;
  }

  bool initSkyBoxProgram() {
    if (!m_skyBoxProgram.initVertexFragment("cube-map.vert", "sky-box.frag")) {
      std::cout << "Failed to init sky box program" << std::endl;
      return false;
    }

    m_skyBoxUniforms.proj = m_skyBoxProgram.getUniformLocation("uProj");
    m_skyBoxUniforms.view = m_skyBoxProgram.getUniformLocation("uView");
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
    m_GBufUniforms.light.direction = m_GBufProgram.getUniformLocation("uLight.direction");
    m_GBufUniforms.light.color = m_GBufProgram.getUniformLocation("uLight.color");
    m_GBufUniforms.light.intensity = m_GBufProgram.getUniformLocation("uLight.intensity");
    m_GBufUniforms.gammaCorrect = m_GBufProgram.getUniformLocation("uGammaCorrect");
    m_GBufUniforms.useDynamicSkinColor = m_GBufProgram.getUniformLocation("uUseDynamicSkinColor");
    m_GBufUniforms.useEnvIrradiance = m_GBufProgram.getUniformLocation("uUseEnvIrradiance");
    m_GBufUniforms.B = m_GBufProgram.getUniformLocation("uB");
    m_GBufUniforms.S = m_GBufProgram.getUniformLocation("uS");
    m_GBufUniforms.F = m_GBufProgram.getUniformLocation("uF");
    m_GBufUniforms.W = m_GBufProgram.getUniformLocation("uW");
    m_GBufUniforms.M = m_GBufProgram.getUniformLocation("uM");
    return true;
  }

  bool initMainProgram() {
    if (!m_mainProgram.initVertexFragment("quad.vert", "main.frag")) {
      std::cout << "Failed to init main program" << std::endl;
      return false;
    }

    m_mainUniforms.light.direction = m_mainProgram.getUniformLocation("uLight.direction");
    m_mainUniforms.light.color = m_mainProgram.getUniformLocation("uLight.color");
    m_mainUniforms.light.intensity = m_mainProgram.getUniformLocation("uLight.intensity");
    m_mainUniforms.light.VPMatrix = m_mainProgram.getUniformLocation("uLight.VPMatrix");
    m_mainUniforms.enableTransmittance = m_mainProgram.getUniformLocation("uEnableTransmittance");
    m_mainUniforms.enableBlur = m_mainProgram.getUniformLocation("uEnableBlur");
    m_mainUniforms.transmittanceStrength =
      m_mainProgram.getUniformLocation("uTransmittanceStrength");
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
    if (!m_finalOutputProgram.initVertexFragment("quad.vert", "final-output.frag")) {
      std::cout << "Failed to init final output program" << std::endl;
      return false;
    }

    m_finalOutputUniforms.gammaCorrect = m_finalOutputProgram.getUniformLocation("uGammaCorrect");
    m_finalOutputUniforms.exposure = m_finalOutputProgram.getUniformLocation("uExposure");
    return true;
  }

private:
  bool initMaps() {
    m_kernelSizeTex = Texture::load("maps/kernelSizeMap.png");
    m_modelSkinColorlessTex = Texture::load("models/james/textures/james_colorless.png");
    m_modelSkinParamMap = Texture::load("models/james/textures/james_skin_params.png");
    m_modelSkinColorLookupTex = Texture::load("maps/skinLookup.png");
    m_paramTex = Texture::load("tex/combined.png");

    return m_kernelSizeTex.isValid() && m_modelSkinColorlessTex.isValid() &&
           m_modelSkinParamMap.isValid() && m_modelSkinColorLookupTex.isValid() &&
           m_paramTex.isValid();
  }

private:
  bool updateMainFBs() {
    releaseFBs(false);
    return initGBufFB() && initMainFB() && initBlurFB();
  }

  void initShadowFB() {
    glCreateFramebuffers(1, &m_shadowFB);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_shadowDepthTex);
    glTextureStorage2D(m_shadowDepthTex, 1, GL_DEPTH_COMPONENT24, ShadowMapSize, ShadowMapSize);

    // Read the texture as grayscale for visualizing, (the shader still only uses the red
    // component).
    glTextureParameteri(m_shadowDepthTex, GL_TEXTURE_SWIZZLE_R, GL_RED);
    glTextureParameteri(m_shadowDepthTex, GL_TEXTURE_SWIZZLE_G, GL_RED);
    glTextureParameteri(m_shadowDepthTex, GL_TEXTURE_SWIZZLE_B, GL_RED);

    glNamedFramebufferTexture(m_shadowFB, GL_DEPTH_ATTACHMENT, m_shadowDepthTex, 0);
    glNamedFramebufferDrawBuffer(m_shadowFB, GL_NONE);
  }

  void initEnvFB() {
    glCreateFramebuffers(1, &m_envMapFB);

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_envColorCubeMap);
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_envIrradianceCubeMap);

    glTextureStorage2D(m_envColorCubeMap, 1, GL_RGB16F, EnvColorSize, EnvColorSize);
    glTextureStorage2D(m_envIrradianceCubeMap, 1, GL_RGB16F, EnvIrradianceSize, EnvIrradianceSize);

    glTextureParameteri(m_envColorCubeMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_envColorCubeMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_envColorCubeMap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_envIrradianceCubeMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_envIrradianceCubeMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_envIrradianceCubeMap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTextureParameteri(m_envColorCubeMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_envColorCubeMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_envIrradianceCubeMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_envIrradianceCubeMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  bool initGBufFB() {
    glCreateFramebuffers(1, &m_GBufFB);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_GBufPosTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_GBufUVTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_GBufNormalTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_GBufAlbedoTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_GBufIrradianceTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_GBufDepthStencilTex);

    glTextureStorage2D(m_GBufPosTex, 1, GL_RGB16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_GBufUVTex, 1, GL_RG16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_GBufNormalTex, 1, GL_RGB16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_GBufAlbedoTex, 1, GL_RGB16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_GBufIrradianceTex, 1, GL_RGB16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_GBufDepthStencilTex, 1, GL_DEPTH24_STENCIL8, m_viewportW, m_viewportH);

    glNamedFramebufferTexture(m_GBufFB, GL_COLOR_ATTACHMENT0, m_GBufPosTex, 0);
    glNamedFramebufferTexture(m_GBufFB, GL_COLOR_ATTACHMENT1, m_GBufUVTex, 0);
    glNamedFramebufferTexture(m_GBufFB, GL_COLOR_ATTACHMENT2, m_GBufNormalTex, 0);
    glNamedFramebufferTexture(m_GBufFB, GL_COLOR_ATTACHMENT3, m_GBufAlbedoTex, 0);
    glNamedFramebufferTexture(m_GBufFB, GL_COLOR_ATTACHMENT4, m_GBufIrradianceTex, 0);
    glNamedFramebufferTexture(m_GBufFB, GL_DEPTH_STENCIL_ATTACHMENT, m_GBufDepthStencilTex, 0);

    unsigned int buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
                              GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4};
    glNamedFramebufferDrawBuffers(m_GBufFB, ARRAY_LENGTH(buffers), buffers);

    return glCheckNamedFramebufferStatus(m_GBufFB, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  }

  bool initMainFB() {
    glCreateFramebuffers(1, &m_mainFB);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_mainFBColorTex);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_mainFBDepthStencilTex);
    glTextureStorage2D(m_mainFBColorTex, 1, GL_RGB16F, m_viewportW, m_viewportH);
    glTextureStorage2D(m_mainFBDepthStencilTex, 1, GL_DEPTH24_STENCIL8, m_viewportW, m_viewportH);

    glNamedFramebufferTexture(m_mainFB, GL_COLOR_ATTACHMENT0, m_mainFBColorTex, 0);
    glNamedFramebufferTexture(m_mainFB, GL_DEPTH_STENCIL_ATTACHMENT, m_mainFBDepthStencilTex, 0);

    return glCheckNamedFramebufferStatus(m_mainFB, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  }

  bool initBlurFB() {
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
    if (releaseFixedSize) {
      if (m_shadowFB) {
        glDeleteTextures(1, &m_shadowDepthTex);
        glDeleteFramebuffers(1, &m_shadowFB);
        m_shadowFB = 0;
        m_shadowDepthTex = 0;
      }

      if (m_envMapFB) {
        glDeleteFramebuffers(1, &m_envMapFB);
        glDeleteTextures(1, &m_envColorCubeMap);
        glDeleteTextures(1, &m_envIrradianceCubeMap);
        m_envMapFB = 0;
        m_envColorCubeMap = 0;
        m_envIrradianceCubeMap = 0;
      }
    }

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
  }

private:
  bool renderEnvCubeMaps() {
    GLuint envColorTex = createEnvTexture(EnvColorMapPath);
    GLuint envIrradianceTex = createEnvTexture(EnvIrradianceMapPath);
    if (envColorTex == GL_INVALID_INDEX || envIrradianceTex == GL_INVALID_INDEX) {
      std::cout << "Failed to load env map" << std::endl;
      return false;
    }

    ShaderProgram envToCubeMapProgram;
    if (!envToCubeMapProgram.initVertexFragment("env-to-cube-map.vert", "env-to-cube-map.frag")) {
      std::cout << "Failed to init env to cube map program" << std::endl;
      return false;
    }

    renderEnvCubeMap(envColorTex, envToCubeMapProgram, m_envColorCubeMap, EnvColorSize);
    renderEnvCubeMap(envIrradianceTex, envToCubeMapProgram, m_envIrradianceCubeMap,
                     EnvIrradianceSize);
    return true;
  }

  static GLuint createEnvTexture(const Path& path) {
    HDRImage image;
    if (!image.load(path, 3))
      return GL_INVALID_INDEX;

    GLuint tex = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGB16F, image.width(), image.height());
    glTextureSubImage2D(tex, 0, 0, 0, image.width(), image.height(), GL_RGB, GL_FLOAT,
                        image.pixels());
    return tex;
  }

  void renderEnvCubeMap(GLuint envTex, const ShaderProgram& envToCubeMapProgram, GLuint outTex,
                        GLsizei outSize) const {
    // https://learnopengl.com/PBR/IBL/Diffuse-irradiance
    Mat4f captureProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    Mat4f captureViews[] = {
      // We need one view for each face of the cube map.
      // clang-format off
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
      // clang-format on
    };

    glBindTextureUnit(0, envTex);
    envToCubeMapProgram.setMat4(envToCubeMapProgram.getUniformLocation("uProj"), captureProj);
    GLint viewLoc = envToCubeMapProgram.getUniformLocation("uView");

    glViewport(0, 0, outSize, outSize);
    glBindFramebuffer(GL_FRAMEBUFFER, m_envMapFB);
    for (int i = 0; i < 6; ++i) {
      envToCubeMapProgram.setMat4(viewLoc, captureViews[i]);

      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, outTex, 0, i);
      glClear(GL_COLOR_BUFFER_BIT);

      m_cube.render(envToCubeMapProgram);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTextureUnit(0, 0);
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

    m_cam.processKeyEvent(e);
    if (!ImGui::GetIO().WantCaptureMouse)
      m_cam.processMouseEvent(e);
  }

private:
  void shadowPass() const {
    glViewport(0, 0, ShadowMapSize, ShadowMapSize);
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

    Mat4f model = m_model.transform();
    Mat4f mvp = m_cam.projectionMatrix() * m_cam.viewMatrix() * model;
    m_GBufProgram.setMat4(m_GBufUniforms.modelMatrix, model);
    m_GBufProgram.setMat4(m_GBufUniforms.MVPMatrix, mvp);
    m_GBufProgram.setMat4(m_GBufUniforms.normalMatrix, glm::transpose(glm::inverse(model)));

    m_GBufProgram.setVec3(m_GBufUniforms.light.position, m_light.position);
    m_GBufProgram.setVec3(m_GBufUniforms.light.direction, m_light.direction);
    m_GBufProgram.setVec3(m_GBufUniforms.light.color, m_light.color);
    m_GBufProgram.setFloat(m_GBufUniforms.light.intensity, m_light.intensity);

    m_GBufProgram.setBool(m_GBufUniforms.gammaCorrect, m_gammaCorrect);
    m_GBufProgram.setBool(m_GBufUniforms.useDynamicSkinColor, m_useDynamicSkinColor);
    m_GBufProgram.setBool(m_GBufUniforms.useEnvIrradiance, m_useEnvIrradiance);

    m_GBufProgram.setFloat(m_GBufUniforms.B, m_B);
    m_GBufProgram.setFloat(m_GBufUniforms.S, m_S);
    m_GBufProgram.setFloat(m_GBufUniforms.F, m_F);
    m_GBufProgram.setFloat(m_GBufUniforms.W, m_W);
    m_GBufProgram.setFloat(m_GBufUniforms.M, m_M);

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glClearStencil(0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glBindTextureUnit(0, m_envIrradianceCubeMap);
    if (m_useDynamicSkinColor)
      glBindTextureUnit(1, m_modelSkinColorlessTex.id);
    else
      m_model.bindMeshAlbedo(0, 1);

    // Normal map is bound by the mesh.

    glBindTextureUnit(3, m_modelSkinParamMap.id);
    glBindTextureUnit(4, m_modelSkinColorLookupTex.id);
    glBindTextureUnit(5, m_paramTex.id);

    m_model.renderForGBuf(m_GBufProgram);

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);

    glBindTextureUnit(3, 0);
    glBindTextureUnit(4, 0);

    glStencilMask(0x00);
    glDisable(GL_STENCIL_TEST);
  }

  void mainPass() const {
    glBlitNamedFramebuffer(m_GBufFB, m_mainFB, 0, 0, m_viewportW, m_viewportH, 0, 0, m_viewportW,
                           m_viewportH, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

    glViewport(0, 0, m_viewportW, m_viewportH);
    glBindFramebuffer(GL_FRAMEBUFFER, m_mainFB);

    glBindTextureUnit(0, m_shadowDepthTex);

    glBindTextureUnit(1, m_GBufPosTex);
    glBindTextureUnit(2, m_GBufUVTex);
    glBindTextureUnit(3, m_GBufNormalTex);
    glBindTextureUnit(4, m_GBufAlbedoTex);
    glBindTextureUnit(5, m_GBufIrradianceTex);

    glBindTextureUnit(6, m_blurFBColorTex);

    m_mainProgram.setVec3(m_mainUniforms.light.direction, m_light.direction);
    m_mainProgram.setVec3(m_mainUniforms.light.color, m_light.color);
    m_mainProgram.setFloat(m_mainUniforms.light.intensity, m_light.intensity);
    m_mainProgram.setMat4(m_mainUniforms.light.VPMatrix, m_light.proj * m_light.view);

    m_mainProgram.setBool(m_mainUniforms.enableTransmittance, m_enableTransmittance);
    m_mainProgram.setBool(m_mainUniforms.enableBlur, m_enableBlur);

    m_mainProgram.setFloat(m_mainUniforms.transmittanceStrength, m_transmittanceStrength);
    m_mainProgram.setFloat(m_mainUniforms.SSSWeight, m_SSSWeight);
    m_mainProgram.setFloat(m_mainUniforms.SSSWidth, m_SSSWidth);
    m_mainProgram.setFloat(m_mainUniforms.SSSNormalBias, m_SSSNormalBias);

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0x00);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glClear(GL_COLOR_BUFFER_BIT);

    m_quad.render(m_mainProgram);

    glDisable(GL_STENCIL_TEST);

    glBindTextureUnit(0, 0);

    glBindTextureUnit(1, 0);
    glBindTextureUnit(2, 0);
    glBindTextureUnit(3, 0);
    glBindTextureUnit(4, 0);
    glBindTextureUnit(5, 0);

    glBindTextureUnit(6, 0);

    if (m_showSkyBox) {
      m_skyBoxProgram.setMat4(m_skyBoxUniforms.proj, m_cam.projectionMatrix());
      m_skyBoxProgram.setMat4(m_skyBoxUniforms.view, m_cam.viewMatrix());

      glBindTextureUnit(0, m_envColorCubeMap);
      m_cube.render(m_skyBoxProgram);
      glBindTextureUnit(0, 0);
    }
  }

  void blurPass() const {
    glBlitNamedFramebuffer(m_GBufFB, m_blurFB, 0, 0, m_viewportW, m_viewportH, 0, 0, m_viewportW,
                           m_viewportH, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

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

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0x00);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glClear(GL_COLOR_BUFFER_BIT);

    m_quad.render(m_blurProgram);

    glDisable(GL_STENCIL_TEST);

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
    glBindTextureUnit(2, 0);
    glBindTextureUnit(3, 0);
  }

  void finalOutputPass() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTextureUnit(0, m_mainFBColorTex);

    m_finalOutputProgram.setBool(m_finalOutputUniforms.gammaCorrect, m_gammaCorrect);
    m_finalOutputProgram.setFloat(m_finalOutputUniforms.exposure, m_exposure);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_quad.render(m_finalOutputProgram);

    glBindTextureUnit(0, 0);
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

  BaseCamera& m_cam = m_trackballCam;
  TrackballCamera m_trackballCam;

  ShaderProgram m_mainProgram;
  MainUniforms m_mainUniforms;
  MaterialMeshModel m_model;
  Texture m_modelSkinColorlessTex;
  Texture m_modelSkinParamMap;
  Texture m_modelSkinColorLookupTex;
  Texture m_paramTex;

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

  // Config.
  bool m_showConfig = false;
  bool m_gammaCorrect = true;
  float m_exposure = 1.0f;
  bool m_showSkyBox = true;
  bool m_useEnvIrradiance = true;
  bool m_useDynamicSkinColor = false;
  bool m_enableTransmittance = true;
  bool m_enableBlur = true;
  float m_transmittanceStrength = 0.75f;
  float m_SSSWeight = 0.5f;
  float m_SSSWidth = 0.015f;
  float m_SSSNormalBias = 0.3f;
  int m_nSamples = 20;
  Vec3f m_falloff = Vec3f(1.0f, 0.37f, 0.3f);
  Vec3f m_strength = Vec3f(0.48f, 0.41f, 0.28f);
  float m_photonPathLength = 2.f;
  unsigned int m_GBufVisTextureIndex = 0;

  float m_B = 0.002f, m_S = 0.75f, m_F = 0.3f, m_W = 0.4f, m_M = 0.3f;

  GLuint m_mainFB = 0;
  GLuint m_mainFBColorTex = 0;
  GLuint m_mainFBDepthStencilTex = 0;
  QuadMesh m_quad;
  ShaderProgram m_finalOutputProgram;
  FinalOutputUniforms m_finalOutputUniforms;

  Texture m_kernelSizeTex;

  // Env map.
  GLuint m_envMapFB = 0;
  GLuint m_envColorCubeMap = 0;
  GLuint m_envIrradianceCubeMap = 0;
  CubeMesh m_cube;
  ShaderProgram m_skyBoxProgram;
  SkyBoxUniforms m_skyBoxUniforms;

  GLuint m_GBufFB = 0;
  GLuint m_GBufPosTex = 0;
  GLuint m_GBufUVTex = 0;
  GLuint m_GBufNormalTex = 0;
  GLuint m_GBufAlbedoTex = 0;
  GLuint m_GBufIrradianceTex = 0;
  GLuint m_GBufDepthStencilTex = 0;
  GBufUniforms m_GBufUniforms;
  ShaderProgram m_GBufProgram;
};

int main(int argc, char** argv) {
  SSSApp app;
  sss::AppContext ctx(app);
  ctx.start(AppName, AppW, AppH);
  return 0;
}
