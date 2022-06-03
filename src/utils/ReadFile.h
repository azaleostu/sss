#pragma once
#ifndef SSS_UTILS_READFILE_H
#define SSS_UTILS_READFILE_H

#include <fstream>
#include <sstream>
#include <string>

namespace sss {

inline bool readFile(const std::string& path, std::string& outData) {
  std::ifstream inFile(path, std::ifstream::in);
  if (!inFile.is_open())
    return false;

  outData = "";
  std::stringstream s;
  s << inFile.rdbuf();
  inFile.close();
  outData = s.str();
  return true;
}

} // namespace sss

#endif
