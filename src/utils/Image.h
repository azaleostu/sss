#pragma once
#ifndef SSS_UTILS_IMAGE_H
#define SSS_UTILS_IMAGE_H

#include "Path.h"

namespace sss {

template <typename T> class Image {
public:
  ~Image() { release(); }

  bool load(const Path& path, int forceChannels = 0);
  void release();

  T* pixels() const { return m_pixels; }

  int width() const { return m_width; }
  int height() const { return m_height; }
  int nbChannels() const { return m_nbChannels; }

private:
  T* m_pixels = nullptr;

  int m_width = 0;
  int m_height = 0;
  int m_nbChannels = 0;
};

using RGBImage = Image<unsigned char>;
using HDRImage = Image<float>;

namespace detail {

template <typename T> struct LoadImage;

template <> struct LoadImage<unsigned char> {
  static unsigned char* call(const char* path, int& width, int& height, int& nbChannels,
                             int desiredChannels);
};

template <> struct LoadImage<float> {
  static float* call(const char* path, int& width, int& height, int& nbChannels,
                     int desiredChannels);
};

const char* getImageError();
void releaseImage(void* pixels);

} // namespace detail

template <typename T> bool Image<T>::load(const Path& path, int forceChannels) {
  release();
  m_pixels =
    detail::LoadImage<T>::call(path.cstr(), m_width, m_height, m_nbChannels, forceChannels);
  if (!m_pixels) {
    std::cout << "Failed to load image \"" << path << "\":\n"
              << detail::getImageError() << std::endl;
    return false;
  }
  return true;
}

template <typename T> void Image<T>::release() {
  if (!m_pixels)
    return;

  detail::releaseImage(m_pixels);
  m_pixels = nullptr;
}

} // namespace sss

#endif
