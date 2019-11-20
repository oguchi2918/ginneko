#ifndef INCLUDED_INPUTIMPL_HPP
#define INCLUDED_INPUTIMPL_HPP

#include <cstring>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"

#include "memory.hpp"

namespace nekolib {
  namespace input {
    class Manager::Impl {
    public:
      Impl() noexcept : x_(0), y_(0), dx_(0), dy_(0), wheel_x_(0), wheel_y_(0),
	mouse_delta_threshold_(100),
	key_state_cur_(0), key_state_prev_(0),
	mouse_state_cur_(0), mouse_state_prev_(0), mouse_focus_(false) {
      }
      
      ~Impl() = default;

      bool update() noexcept {
	wheel_x_ = wheel_y_ = 0;

	// SDL event loop
	SDL_Event ev;

	while (SDL_PollEvent(&ev)) {
	  // forward to imgui
	  ImGui_ImplSDL2_ProcessEvent(&ev);
	  
	  switch (ev.type) {
	  case SDL_QUIT:
	    return false;
	    break;
	  case SDL_KEYUP:
	    if (ev.key.keysym.sym == SDLK_ESCAPE) {
	      return false;
	    }
	    break;
	  case SDL_WINDOWEVENT:
	    if (ev.window.event == SDL_WINDOWEVENT_ENTER) {
	      mouse_focus_ = true;
	    } else if (ev.window.event == SDL_WINDOWEVENT_LEAVE) {
	      mouse_focus_ = false;
	    }
	    break;
	  case SDL_MOUSEWHEEL:
	    wheel_x_ += ev.wheel.x;
	    wheel_y_ += ev.wheel.y;
	    break;
	  // case SDL_MOUSEBUTTONDOWN:
	  //   fprintf(stderr, "mouse button down %d\n", ev.button.button);
	  //   break;
	  default:
	    break;
	  }
	}

	// backup and get keyboard states
	key_state_prev_ = key_state_cur_;
	int keys_num;
	const Uint8* state = SDL_GetKeyboardState(&keys_num);
	key_state_cur_ = static_cast<unsigned char*>(nekolib::memory::Manager::alloc(sizeof(Uint8) * keys_num));
	::memcpy(key_state_cur_, state, sizeof(Uint8) * keys_num);
	
	if (mouse_focus_) {
	  int x_prev = x_, y_prev = y_;
	  mouse_state_prev_ = mouse_state_cur_;
	  mouse_state_cur_ = SDL_GetMouseState(&x_, &y_);

	  dx_ = x_ - x_prev;
	  dy_ = y_ - y_prev;

	  if (dx_ >= mouse_delta_threshold_ || dx_ <= -mouse_delta_threshold_ ||
	      dy_ >= mouse_delta_threshold_ || dy_ <= -mouse_delta_threshold_) {
	    dx_ = dy_ = 0;
	  }
	} else {
	  dx_ = dy_ = 0;
	}

	return true;
      }
      
      bool key_pushed(int i) noexcept {
	SDL_Scancode sc = SDL_GetScancodeFromKey(i);
	return key_state_cur_[sc] != 0;
      }
      
      bool key_triggered(int i) noexcept {
	SDL_Scancode sc = SDL_GetScancodeFromKey(i);
	return (key_state_cur_[sc] != 0) && (key_state_prev_[sc] == 0);
      }
      
      bool mouse_pushed(int i) noexcept {
	return (mouse_state_cur_ & SDL_BUTTON(i)) != 0;
      }
      
      bool mouse_triggered(int i) noexcept {
	return (mouse_state_cur_ & SDL_BUTTON(i)) != 0 && (mouse_state_prev_ & SDL_BUTTON(i)) == 0;
      }

      int x_, y_;
      int dx_, dy_;
      int wheel_x_, wheel_y_;
      int mouse_delta_threshold_;
    private:
      unsigned char* key_state_cur_;
      unsigned char* key_state_prev_;
      unsigned int mouse_state_cur_;
      unsigned int mouse_state_prev_;
      bool mouse_focus_;
    };
  }
}

#endif // INCLUDED_INPUTIMPL_HPP
