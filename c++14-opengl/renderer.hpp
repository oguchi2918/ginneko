#ifndef INCLUDED_RENDERER_HPP
#define INCLUDED_RENDERER_HPP

namespace nekolib {
  namespace renderer {
    // utility
    void regist_debug_callback();
    
    // singleton class for screen info and viewport
    class ScreenManager {
    public:
      ScreenManager() = delete;
      ~ScreenManager() = delete;

      static void init(int, int); // 初期化
      static void reset(int, int); // 再設定
      static int width() noexcept;
      static int height() noexcept;
      static double aspectd() noexcept;
      static float aspectf() noexcept;
    };

    // framerate(average) and delta_t(last 1 frame) info
    // imgui内にも類似品があるのでこれからはそっち使う方針で
    class FramerateCounter {
    public:
      FramerateCounter() = delete;
      ~FramerateCounter() = delete;

      static bool next_frame(); // You must call this func every frame
      static void reset() noexcept; // reset current frame count

      static float mean_framerate() noexcept;
      static unsigned long delta_t() noexcept;

      static void min_report_frames(unsigned long) noexcept;
      static void min_report_milliseconds(unsigned long) noexcept;
    };

  }
}

#endif // INCLUDED_RENDERER_HPP
