#version 460

#define HEAD_CIRCUMFERENCE_MM 500 // mm <=> texture is 50cm by 50cm

// https://www.ncbi.nlm.nih.gov/pmc/articles/PMC7249824/
// #define MEAN_PHOTON_PATH_LENGTH 2 // mm

// #define SSSS_FOLLOW_SURFACE 1

in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D uColorMap;
layout(binding = 1) uniform sampler2D uDepthMap;
layout(binding = 2) uniform sampler2D uCustomMap;
layout(binding = 3) uniform sampler2D uUVMap;

uniform float uFovy;
uniform float uSSSWidth;

#define MAX_NUM_SAMPLES 50
uniform int uNumSamples;
uniform vec3 uFalloff;
uniform vec3 uStrength;
uniform float uPhotonPathLength; // mm

vec4 kernel[MAX_NUM_SAMPLES];

// https://github.com/iryoku/separable-sss/blob/master/Demo/Code/SeparableSSS.cpp#L172
vec3 gaussian(float variance, float r, vec3 falloff) {
  vec3 g;
  for (int i = 0; i < 3; i++) {
    float rr = r / (0.001f + falloff[i]);
    g[i] = exp((-(rr * rr)) / (2.0f * variance)) / (2.0f * 3.14f * variance);
  }
  return g;
}

vec3 profile(float r, vec3 falloff) {
  return
    0.100f * gaussian(0.0484f, r, falloff) +
    0.118f * gaussian(0.187f, r, falloff) +
    0.113f * gaussian(0.567f, r, falloff) +
    0.358f * gaussian(1.99f, r, falloff) +
    0.078f * gaussian(7.41f, r, falloff);
}

void calculateKernel(int nSamples, vec3 falloff, vec3 strength) {
  const float RANGE = nSamples > 20 ? 3.0f : 2.0f;
  const float EXPONENT = 2.0f;

  // Calculate the offsets:
  float step = 2.0f * RANGE / (nSamples - 1);
  for (int i = 0; i < nSamples; i++) {
    float o = -RANGE + float(i) * step;
    float sign = o < 0.0f ? -1.0f : 1.0f;
    kernel[i].w = RANGE * sign * abs(pow(o, EXPONENT)) / pow(RANGE, EXPONENT);
  }

  // Calculate the weights:
  for (int i = 0; i < nSamples; i++) {
    float w0 = i > 0 ? abs(kernel[i].w - kernel[i - 1].w) : 0.0f;
    float w1 = i < nSamples - 1 ? abs(kernel[i].w - kernel[i + 1].w) : 0.0f;
    float area = (w0 + w1) / 2.0f;
    vec3 t = area * profile(kernel[i].w, falloff);
    kernel[i].x = t.x;
    kernel[i].y = t.y;
    kernel[i].z = t.z;
  }

  // We want the offset 0.0 to come first:
  vec4 t = kernel[nSamples / 2];
  for (int i = nSamples / 2; i > 0; i--)
    kernel[i] = kernel[i - 1];
  kernel[0] = t;

  // Calculate the sum of the weights, we will need to normalize them below:
  vec3 sum = vec3(0.0f, 0.0f, 0.0f);
  for (int i = 0; i < nSamples; i++)
    sum += vec3(kernel[i]);

  // Normalize the weights:
  for (int i = 0; i < nSamples; i++) {
    kernel[i].x /= sum.x;
    kernel[i].y /= sum.y;
    kernel[i].z /= sum.z;
  }

  // Tweak them using the desired strength. The first one is:
  // lerp(1.0, kernel[0].rgb, strength)
  kernel[0].x = (1.0f - strength.x) * 1.0f + strength.x * kernel[0].x;
  kernel[0].y = (1.0f - strength.y) * 1.0f + strength.y * kernel[0].y;
  kernel[0].z = (1.0f - strength.z) * 1.0f + strength.z * kernel[0].z;

  // The others:
  // lerp(0.0, kernel[0].rgb, strength)
  for (int i = 1; i < nSamples; i++) {
    kernel[i].x *= strength.x;
    kernel[i].y *= strength.y;
    kernel[i].z *= strength.z;
  }
}

// http://www.iryoku.com/separable-sss/
vec4 applyBlur(float fovy, float sssWidth, int nSamples, vec2 uv, vec4 colorM, vec2 dir, float fwRel) {
  // #############################################
  // Fetch linear depth of current pixel:
  float depthM = texture(uDepthMap, TexCoords).r;

  // ############################################# old
  // Calculate the sssWidth scale (1.0 for a unit plane sitting on the projection window):
  // float distanceToProjectionWindow = 1.0 / tan(0.5 * radians(fovy));
  // float scale = distanceToProjectionWindow / depthM;
  // Calculate the final step to fetch the surrounding pixels:
  // vec2 finalStep = sssWidth * scale * dir;
  // finalStep *= 1.0 / 3.0; // Divide by 3 as the kernels range from -3 to 3.
  // ############################################# new
  vec2 finalStep = dir * (sssWidth * fwRel);
  // finalStep *= colorM.a;
  finalStep *= 1.0 / 3.0;
  // #############################################

  // Accumulate the center sample:
  vec4 colorBlurred = colorM;
  colorBlurred.rgb *= kernel[0].rgb;

  // Accumulate the other samples:
  nSamples = min(nSamples, MAX_NUM_SAMPLES);
  for (int i = 1; i < nSamples; i++) {
    // Fetch color and depth for current sample:
    vec2 offset = TexCoords + kernel[i].a * finalStep;
    vec4 color = texture(uColorMap, offset);

    // ############################################# old
    // #if SSSS_FOLLOW_SURFACE == 1
    // If the difference in depth is huge, we lerp color back to "colorM":
    // float depth = texture(uDepthMap, offset).r;
    // float s = clamp(300.0f * distanceToProjectionWindow * sssWidth * abs(depthM - depth), 0.0, 1.0);
    // color.rgb = mix(color.rgb, colorM.rgb, s);
    // #endif
    // ############################################# new
    float depth = texture(uDepthMap, offset).r;
    if (abs(depthM - depth) > 0.0002)
      color.rgb = colorM.rgb;
    // #############################################

    // Accumulate:
    colorBlurred.rgb += kernel[i].rgb * color.rgb;
  }

  return colorBlurred;
}

void main() {
  vec2 uv = texture(uUVMap, TexCoords).rg;
  float scale = HEAD_CIRCUMFERENCE_MM / uPhotonPathLength;

  vec2 fwuv = clamp(fwidth(uv),0.001,1000.0);
  float fwz = fwidth(texture(uDepthMap, TexCoords).r);
  float fwRel = 1 / (scale * distance(vec3(0.0), vec3(fwuv, fwz))); // with depth
  // float fwRel = 1 / (scale * sqrt(fwuv.x * fwuv.x + fwuv.y * fwuv.y)); // without depth

  int nSamples = uNumSamples; // int((texture(uCustomMap, uv).r / 1) * 1 + 20);
  vec3 strength = uStrength; // texture(uCustomMap, uv).ggg;
  float sssWidth = uSSSWidth; // texture(uCustomMap, uv).r * 100;
  calculateKernel(nSamples, uFalloff, strength);

  // Fetch color of current pixel:
  vec4 colorM = texture(uColorMap, TexCoords);

  // Not exactly a two-pass blur, but the results are almost the same.
  colorM = applyBlur(uFovy, sssWidth, nSamples, uv, colorM, vec2(1.0, 0.0), fwRel);
  FragColor = applyBlur(uFovy, sssWidth, nSamples, uv, colorM, vec2(0.0, 1.0), fwRel); // normal blur

  // DEBUG
  // FragColor = texture(uCustomMap, uv); // map texture directly on face
  // FragColor = vec4(fwRel, 0, 0, colorM.a);
}
