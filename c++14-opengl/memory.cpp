#include <cassert>

#include "memory.hpp"
#include "memoryimpl.hpp"

namespace nekolib {
  namespace memory {
    static const unsigned FRAME_STACK_SIZE = 2048;
    static unsigned char g_stack[FRAME_STACK_SIZE * 2];
    static Manager::Impl* g_impl = 0;

    void Manager::init()
    {
      g_impl = new Manager::Impl(g_stack, FRAME_STACK_SIZE);
    }

    void Manager::update() noexcept
    {
      assert(g_impl);
      g_impl->update();
    }

    void* Manager::alloc(unsigned size_bytes) noexcept
    {
      assert(g_impl);
      return g_impl->alloc(size_bytes);
    }
  }
}
