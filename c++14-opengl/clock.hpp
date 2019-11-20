#ifndef INCLUDED_CLOCK_HPP
#define INCLUDED_CLOCK_HPP

namespace nekolib {
  namespace clock {

    class Manager {
    public:
      static void init() noexcept;
      static void update() noexcept;
      
      Manager() = delete;
      ~Manager() = delete;
    };

    // 毎フレーム更新される時計クラス
    class Clock {
    public:
      static Clock create(float start_time_seconds = 0.f, bool start_pause = false) noexcept;
      static float calc_delta_seconds(const Clock& cur, const Clock& start) noexcept;

      // 複製時計を返す
      Clock dup() const noexcept;
      // 現在時刻で止まった複製時計を返す
      Clock snapshot() const noexcept; 

      uint64_t time_cycles() const noexcept;
      void pause(bool) noexcept;
      void scale(float) noexcept;

      // 以下、参照回数計測クラスのテンプレート
      Clock() noexcept;
      ~Clock() noexcept;
      Clock(const Clock&) noexcept;
      Clock(Clock&&) noexcept;
      Clock& operator=(const Clock&) noexcept;
      Clock& operator=(Clock&&) noexcept;
      bool operator==(const Clock&) const noexcept;
      bool operator!=(const Clock&) const noexcept;

      class Impl;
    private:
      Impl* impl_;
      void release() noexcept;
    };
  }
}

#endif // INCLUDED_CLOCK_HPP
