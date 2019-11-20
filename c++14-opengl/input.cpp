#include <cassert>

#include "input.hpp"
#include "inputimpl.hpp"

namespace nekolib {
  namespace input {
    static Manager::Impl* g_impl = 0;

    Manager::Manager() noexcept = default;

    void Manager::init()
    {
      assert(!g_impl);
      g_impl = new Manager::Impl();
    }

    bool Manager::update() noexcept
    {
      return g_impl->update();
    }

    Manager Manager::instance() noexcept
    {
      return Manager();
    }

    Keyboard Manager::keyboard() const noexcept
    {
      return Keyboard();
    }

    Mouse Manager::mouse() const noexcept
    {
      return Mouse();
    }

    bool Keyboard::pushed(int i) const noexcept
    {
      return g_impl->key_pushed(i);
    }

    bool Keyboard::triggered(int i) const noexcept
    {
      return g_impl->key_triggered(i);
    }

    bool Mouse::pushed(Mouse::Button i) const noexcept
    {
      return g_impl->mouse_pushed(static_cast<int>(i));
    }

    bool Mouse::triggered(Mouse::Button i) const noexcept
    {
      return g_impl->mouse_triggered(static_cast<int>(i));
    }

    int Mouse::wheel_x() const noexcept
    {
      return g_impl->wheel_x_;
    }

    int Mouse::wheel_y() const noexcept
    {
      return g_impl->wheel_y_;
    }

    int Mouse::x() const noexcept
    {
      return g_impl->x_;
    }

    int Mouse::y() const noexcept
    {
      return g_impl->y_;
    }

    int Mouse::dx() const noexcept
    {
      return g_impl->dx_;
    }

    int Mouse::dy() const noexcept
    {
      return g_impl->dy_;
    }

    void Mouse::set_threshold(unsigned int th) noexcept
    {
      g_impl->mouse_delta_threshold_ = static_cast<int>(th);
    }
  }
}
