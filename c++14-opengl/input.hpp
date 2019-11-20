#ifndef INCLUDED_INPUT_HPP
#define INCLUDED_INPUT_HPP

#include <SDL2/SDL.h>

namespace nekolib {
  namespace input {
    class Keyboard {
    public:
      bool pushed(int i) const noexcept;
      bool triggered(int i) const noexcept;
    };

    class Mouse {
    public:
      enum class Button : int {
	LEFT = 1,   //SDL_BUTTON_LEFT,
	MIDDLE = 2, // SDL_BUTTON_MIDDLE,
	RIGHT = 3,  // SDL_BUTTON_RIGHT,
      };
      
      int x() const noexcept;
      int y() const noexcept;
      int dx() const noexcept; // 1 frame delta x
      int dy() const noexcept; // 1 frame delta y
      int wheel_x() const noexcept;
      int wheel_y() const noexcept;

      bool pushed(Button) const noexcept;
      bool triggered(Button) const noexcept;

      void set_threshold(unsigned int) noexcept;
    };
    
    class Manager {
    public:
      Manager() noexcept;
      static Manager instance() noexcept;

      Mouse mouse() const noexcept;
      Keyboard keyboard() const noexcept;

      static void init();
      static bool update() noexcept;
      class Impl;
    };
  }
}

#endif // INCLUDED_INPUT_HPP
