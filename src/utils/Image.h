#pragma once
#ifndef SSS_UTILS_IMAGE_H
#define SSS_UTILS_IMAGE_H

#include "Path.h"

namespace sss {

class Image {
public:
  ~Image() { release(); }

  bool load(const Path& path);
  void release();

  unsigned char* pixels() const { return m_pixels; }
  int width() const { return m_width; }
  int height() const { return m_height; }
  int nbChannels() const { return m_nbChannels; }

private:
  unsigned char* m_pixels = nullptr;
  int m_width = 0;
  int m_height = 0;
  int m_nbChannels = 0;
};

} // namespace sss

#endif
