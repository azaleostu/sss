#pragma once
#ifndef SSS_UTILS_PATH_H
#define SSS_UTILS_PATH_H

#include <istream>
#include <ostream>
#include <string>
#include <utility>

namespace sss {

class Path {
public:
#ifdef _WIN32
  static const char Sep = '\\';
#else
  static const char Sep = '/';
#endif
  Path() = default;

  /*implicit*/ Path(const char* str)
    : m_str(str) {
    format();
  }
  /*implicit*/ Path(std::string str)
    : m_str(std::move(str)) {
    format();
  }

  bool isEmpty() const { return m_str.empty(); }

  const std::string& str() const { return m_str; }
  const char* cstr() const { return str().c_str(); }

  Path dir() const {
    const size_t pos = m_str.find_last_of(Sep);
    if (pos == std::string::npos)
      return {};
    return m_str.substr(0, pos);
  }

  std::string filePath() const {
    const size_t pos = m_str.find_last_of(Sep);
    if (pos == std::string::npos)
      return m_str;
    return m_str.substr(pos + 1);
  }

  std::string ext() const {
    const size_t pos = m_str.find_last_of('.');
    if (pos == std::string::npos || pos == 0)
      return "";
    return m_str.substr(pos + 1);
  }

  std::string fileName() const {
    std::string path = filePath();
    const size_t pos = path.find_last_of('.');
    if (pos != std::string::npos && pos != 0)
      path = path.substr(0, pos);
    return path;
  }

  Path operator+(const Path& other) const {
    if (isEmpty())
      return other;
    else if (other.isEmpty())
      return *this;

    Path res = *this;
    if (other.m_str.front() != Sep)
      res.m_str += Sep;
    res.m_str += other.m_str;
    return res;
  }

  bool operator==(const Path& other) const { return m_str == other.m_str; }
  bool operator!=(const Path& other) const { return !operator==(other); }

  friend std::ostream& operator<<(std::ostream& os, const Path& p) { return os << p.m_str; }
  friend std::istream& operator>>(std::istream& is, Path& p) {
    is >> p.m_str;
    p.format();
    return is;
  }

private:
  void format() {
    for (char& c : m_str) {
      if (c == '\\' || c == '/')
        c = Sep;
    }
    while (!m_str.empty() && m_str.back() == Sep)
      m_str.pop_back();
  }

private:
  std::string m_str;
};

} // namespace sss

#endif
