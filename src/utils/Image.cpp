#include "Image.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

namespace sss {

bool Image::load(const Path& path) {
  release();
  m_pixels = stbi_load(path.cstr(), &m_width, &m_height, &m_nbChannels, 0);
  if (!m_pixels) {
    std::cout << "Failed to load image \"" << path << "\":\n" << stbi_failure_reason() << std::endl;
    return false;
  }
  return true;
}

void Image::release() {
  if (!m_pixels)
    return;
  stbi_image_free(m_pixels);
  m_pixels = nullptr;
}

} // namespace sss
