#version 450

#define SSSS_N_SAMPLES 11
#define SSSS_FOLLOW_SURFACE 1

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D uColorMap;
uniform sampler2D uDepthMap;
uniform float fovy;
uniform float sssWidth;

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

vec4 SSSSBlurPS(vec2 texcoord, float sssWidth, vec2 dir, bool initStencil) {
  // Fetch color of current pixel:
  vec4 colorM = texture(uColorMap, texcoord);

  // Initialize the stencil buffer in case it was not already available:
  if (initStencil)
    if (colorM.a == 0.0) discard;

  // Fetch linear depth of current pixel:
  float depthM = texture(uDepthMap, texcoord).r;

  // Calculate the sssWidth scale (1.0 for a unit plane sitting on the
  // projection window):
  float distanceToProjectionWindow = 1.0 / tan(0.5 * radians(fovy));
  float scale = distanceToProjectionWindow / depthM;

  // Calculate the final step to fetch the surrounding pixels:
  vec2 finalStep = sssWidth * scale * dir;
  finalStep *= colorM.a;// Modulate it using the alpha channel.
  finalStep *= 1.0 / 3.0;// Divide by 3 as the kernels range from -3 to 3.

  // Accumulate the center sample:
  vec4 colorBlurred = colorM;
  colorBlurred.rgb *= kernel[0].rgb;

  // Accumulate the other samples:
  for (int i = 1; i < SSSS_N_SAMPLES; i++) {
    // Fetch color and depth for current sample:
    vec2 offset = texcoord + kernel[i].a * finalStep;
    vec4 color = texture(uColorMap, offset);

    #if SSSS_FOLLOW_SURFACE == 1
    // If the difference in depth is huge, we lerp color back to "colorM":
    float depth = texture(depthTex, offset).r;
    float s = clamp(300.0f * distanceToProjectionWindow *
    sssWidth * abs(depthM - depth), 0.0, 1.0);
    color.rgb = mix(color.rgb, colorM.rgb, s);
    #endif

    // Accumulate:
    colorBlurred.rgb += kernel[i].rgb * color.rgb;
  }

  return colorBlurred;
}

void main() {
  vec2 dir = vec2(1.0, 0.0);
  FragColor = SSSSBlurPS(TexCoords, sssWidth, dir, true);
  dir = vec2(0.0, 1.0);
  FragColor = SSSSBlurPS(TexCoords, sssWidth, dir, false);
}
