#version 460

#define SSSS_N_SAMPLES 11
#define SSSS_FOLLOW_SURFACE 1

in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D uColorMap;
layout(binding = 1) uniform sampler2D uDepthMap;

uniform float uFovy;
uniform float uSSSWidth;

vec4 kernel[] = {
  vec4(0.560479, 0.669086, 0.784728, 0),
  vec4(0.00471691, 0.000184771, 5.07566e-005, -2),
  vec4(0.0192831, 0.00282018, 0.00084214, -1.28),
  vec4(0.03639, 0.0130999, 0.00643685, -0.72),
  vec4(0.0821904, 0.0358608, 0.0209261, -0.32),
  vec4(0.0771802, 0.113491, 0.0793803, -0.08),
  vec4(0.0771802, 0.113491, 0.0793803, 0.08),
  vec4(0.0821904, 0.0358608, 0.0209261, 0.32),
  vec4(0.03639, 0.0130999, 0.00643685, 0.72),
  vec4(0.0192831, 0.00282018, 0.00084214, 1.28),
  vec4(0.00471691, 0.000184771, 5.07565e-005, 2),
};

vec4 applyBlur(vec4 colorM, vec2 dir) {
  // Fetch linear depth of current pixel:
  float depthM = texture(uDepthMap, TexCoords).r;

  // Calculate the sssWidth scale (1.0 for a unit plane sitting on the
  // projection window):
  float distanceToProjectionWindow = 1.0 / tan(0.5 * radians(uFovy));
  float scale = distanceToProjectionWindow / depthM;

  // Calculate the final step to fetch the surrounding pixels:
  vec2 finalStep = uSSSWidth * scale * dir;
  finalStep *= colorM.a; // Modulate it using the alpha channel.
  finalStep *= 1.0 / 3.0; // Divide by 3 as the kernels range from -3 to 3.

  // Accumulate the center sample:
  vec4 colorBlurred = colorM;
  colorBlurred.rgb *= kernel[0].rgb;

  // Accumulate the other samples:
  for (int i = 1; i < SSSS_N_SAMPLES; i++) {
    // Fetch color and depth for current sample:
    vec2 offset = TexCoords + kernel[i].a * finalStep;
    vec4 color = texture(uColorMap, offset);

    #if SSSS_FOLLOW_SURFACE == 1
    // If the difference in depth is huge, we lerp color back to "colorM":
    float depth = texture(uDepthMap, offset).r;
    float s = clamp(300.0f * distanceToProjectionWindow * uSSSWidth * abs(depthM - depth), 0.0, 1.0);
    color.rgb = mix(color.rgb, colorM.rgb, s);
    #endif

    // Accumulate:
    colorBlurred.rgb += kernel[i].rgb * color.rgb;
  }

  return colorBlurred;
}

void main() {
  // Fetch color of current pixel:
  vec4 colorM = texture(uColorMap, TexCoords);
  colorM = applyBlur(colorM, vec2(1.0, 0.0));
  FragColor = applyBlur(colorM, vec2(0.0, 1.0));
}
