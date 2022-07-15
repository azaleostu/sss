#include "Image.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

namespace sss {
namespace detail {

unsigned char* LoadImage<unsigned char>::call(const char* path, int& width, int& height,
                                              int& nbChannels, int desiredChannels) {
  return stbi_load(path, &width, &height, &nbChannels, desiredChannels);
}

float* LoadImage<float>::call(const char* path, int& width, int& height, int& nbChannels,
                              int desiredChannels) {
  return stbi_loadf(path, &width, &height, &nbChannels, desiredChannels);
}

} // namespace detail

const char* detail::getImageError() { return stbi_failure_reason(); }

void detail::releaseImage(void* pixels) { stbi_image_free(pixels); }

} // namespace sss
