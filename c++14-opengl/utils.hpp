#ifndef INCLUDED_UTILS_HPP
#define INCLUDED_UTILS_HPP

#include <algorithm>
#include <cassert>
#include <SDL2/SDL.h>

namespace nekolib {
  template <typename T>
  T clamp(T x, T low, T high) {
    assert(low <= high);
    return std::min(std::max(x, low), high);
  }

  int wrap(int, int, int);
  float wrap(float, float, float);
  
  namespace renderer {
    bool check_gl_error(const char*, int);
    bool check_fbo_status(const char*, int);
    void dump_gl_info(bool dump_extensions = false);
  }
}

#endif // INCLUDED_UTILS_HPP
