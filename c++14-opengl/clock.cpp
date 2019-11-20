#include <cstdint>
#include <SDL2/SDL_timer.h>

#include "clock.hpp"
#include "utils.hpp"

namespace {
  static const int MAX_CLOCKS = 256; // 生成可能な時計総数
  
  // 以下の2変数はnekolib::clock::Manager::init()で初期化される
  static float g_cycles_per_second = 0;
  static uint64_t g_time_cycles = 0;
}

namespace nekolib {
  namespace clock {
    static inline uint64_t seconds_to_cycles(float time_seconds) noexcept {
      return static_cast<uint64_t>(time_seconds * g_cycles_per_second);
    }

    // caution -- 短い経過時間を秒単位に変換するためだけに使う
    static inline float cycles_to_seconds(uint64_t time_cycles) noexcept {
      return static_cast<float>(time_cycles) / g_cycles_per_second;
    }

    class Clock::Impl {
    public:
      uint64_t time_cycles_;
      float time_scale_;
      bool paused_;
      Impl* next_;
      Impl* prev_;
      unsigned count_;

      Impl() noexcept : time_cycles_(0), time_scale_(1.f), paused_(false),
	                next_(nullptr), prev_(nullptr),
	                count_(0) {
      }

      ~Impl() noexcept {
      }

      void refer() noexcept { ++count_; }
      void release() noexcept { --count_; }
      unsigned count() const noexcept { return count_; }
      
      float calc_delta_seconds(const Impl& other) const noexcept {
	uint64_t dt = time_cycles_ - other.time_cycles_;
	return cycles_to_seconds(dt);
      }

      void update(uint64_t real_dt_cycles) noexcept {
	if (!paused_) {
	  uint64_t scaled_cycles = real_dt_cycles * time_scale_;
	  time_cycles_ += scaled_cycles;
	}
      }
    };

    // 実装は静的配列をフリーリストと使用済リストで管理した
    // タスクシステム的何かをスマートポインタに包んで返す感じで
    static Clock::Impl g_clock_pool[MAX_CLOCKS];
    static Clock::Impl g_free_list;
    static Clock::Impl g_used_list;

    static void free_clock(Clock::Impl* clock) noexcept
    {
      Clock::Impl* tmp = g_free_list.next_;
      g_free_list.next_->prev_ = clock;
      g_free_list.next_ = clock;

      clock->next_->prev_ = clock->prev_;
      clock->prev_->next_ = clock->next_;

      clock->prev_ = &g_free_list;
      clock->next_ = tmp;
    }

    static Clock::Impl* alloc_clock() noexcept
    {
      if (g_free_list.next_ == &g_free_list) {
	return nullptr;
      }

      Clock::Impl* ret = g_free_list.next_;
      Clock::Impl* tmp = g_used_list.next_;
      g_used_list.next_->prev_ = ret;
      g_used_list.next_ = ret;

      ret->next_->prev_ = ret->prev_;
      ret->prev_->next_ = ret->next_;

      ret->prev_ = &g_used_list;
      ret->next_ = tmp;

      return ret;
    }

    void Manager::init() noexcept
    {
      g_cycles_per_second = SDL_GetPerformanceFrequency();
      g_time_cycles = SDL_GetPerformanceCounter();
      for (int i = 0; i < MAX_CLOCKS; ++i) {
	g_clock_pool[i].next_ = &g_clock_pool[i + 1];
	g_clock_pool[i].prev_ = &g_clock_pool[i - 1]; 
      }
      g_clock_pool[0].prev_ = g_clock_pool[MAX_CLOCKS - 1].next_ = &g_free_list;
      g_free_list.next_ = &g_clock_pool[0];
      g_free_list.prev_ = &g_clock_pool[MAX_CLOCKS - 1];
      g_used_list.next_ = g_used_list.prev_ = &g_used_list;
    }

    void Manager::update() noexcept
    {
      uint64_t cur = SDL_GetPerformanceCounter();
      uint64_t delta_t = cur - g_time_cycles;

      Clock::Impl* tmp = g_used_list.next_;
      while (tmp != &g_used_list) {
	tmp->update(delta_t);
	tmp = tmp->next_;
      }
      g_time_cycles = cur;
    }

    Clock Clock::create(float start_time_seconds, bool start_pause) noexcept
    {
      Clock r;
      r.impl_ = alloc_clock();
      if (r.impl_) {      // 配置new
	r.impl_->refer(); // 参照回数を1にするのを忘れずに
	r.impl_->time_cycles_ = seconds_to_cycles(start_time_seconds);
	r.impl_->time_scale_ = 1.f;
	r.impl_->paused_ = start_pause;
      }

      return r;
    }

    Clock Clock::dup() const noexcept
    {
      assert(impl_);
      Clock r;
      r.impl_ = alloc_clock();
      if (r.impl_) {      // 配置new
	r.impl_->refer(); // 参照回数を1にするのを忘れずに
	r.impl_->time_cycles_ = impl_->time_cycles_;
	r.impl_->time_scale_ = impl_->time_scale_;
	r.impl_->paused_ = impl_->paused_;
      }

      return r;
    }

    Clock Clock::snapshot() const noexcept
    {
      Clock r(dup());
      r.pause(true);
      return r;
    }

    uint64_t Clock::time_cycles() const noexcept
    {
      assert(impl_);
      return impl_->time_cycles_;
    }

    float Clock::calc_delta_seconds(const Clock& cur, const Clock& start) noexcept
    {
      assert(cur.impl_ && start.impl_);
      if (cur.impl_ == start.impl_) {
	return 0.f;
      }
      uint64_t dt = cur.impl_->time_cycles_ - start.impl_->time_cycles_;
      return cycles_to_seconds(dt);
    }

    void Clock::pause(bool flag) noexcept
    {
      assert(impl_);
      impl_->paused_ = flag;
    }

    void Clock::scale(float time_scale) noexcept
    {
      assert(impl_);
      impl_->time_scale_ = time_scale;
    }

    Clock::Clock() noexcept : impl_(nullptr)
    {
    }

    Clock::~Clock() noexcept {
      release();
    }

    Clock::Clock(const Clock& o) noexcept : impl_(o.impl_) {
      if (impl_) {
	impl_->refer();
      }
    }

    Clock::Clock(Clock&& o) noexcept : impl_(o.impl_) {
      o.impl_ = nullptr;
    }

    void Clock::release() noexcept {
      if (impl_) {
	impl_->release();
	if (impl_->count() == 0) {
	  free_clock(impl_);
	}
      }
      impl_ = nullptr;
    }

    Clock& Clock::operator=(const Clock& o) noexcept {
      release();
      impl_ = o.impl_;
      if (impl_) {
	impl_->refer();
      }
      return *this;
    }

    Clock& Clock::operator=(Clock&& o) noexcept {
      release();
      impl_ = o.impl_; o.impl_ = nullptr;
      return *this;
    }

    bool Clock::operator==(const Clock& o) const noexcept {
      return impl_ == o.impl_;
    }
    
    bool Clock::operator!=(const Clock& o) const noexcept {
      return impl_ != o.impl_;
    }
  }
}

/*
#include <iostream>
namespace {
  void print_list_status()
  {
    using namespace nekolib::clock;
    std::cout << "free_list\n";
    Clock::Impl* tmp = &g_free_list;
    do {
      std::cout << tmp << "->";
      tmp = tmp->next_;
    } while (tmp != &g_free_list);
    std::cout << std::endl;
    do {
      std::cout << tmp << "<-";
      tmp = tmp->prev_;
    } while (tmp != &g_free_list);
    std::cout << std::endl;
    std::cout << "used_list\n";
    tmp = &g_used_list;
    do {
      std::cout << tmp << "->";
      tmp = tmp->next_;
    } while (tmp != &g_used_list);
    std::cout << std::endl;
    do {
      std::cout << tmp << "<-";
      tmp = tmp->prev_;
    } while (tmp != &g_used_list);
    std::cout << std::endl;
  }
}
*/
