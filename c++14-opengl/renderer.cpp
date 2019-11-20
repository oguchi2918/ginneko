#include <cstdio>
#include <cassert>
#include <glad/glad.h>
#include <SDL2/SDL_timer.h>

#include "renderer.hpp"

namespace nekolib {
  namespace renderer {
    void regist_debug_callback()
    {
      GLint flags;
      glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
      if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      
	glDebugMessageCallback
	  ([](GLenum source, GLenum type, GLuint id, GLenum severity,
	      GLsizei, const char* mes, const void*) {

	    // ignore these non-significant error codes
	    if(id == 131169 || id == 131185 || id == 131186 || id == 131218 || id == 131204) {
	      return;
	    }
	    fprintf(stderr, "----------------\n");
	    fprintf(stderr, "Debug message(%d): %s\n", id, mes);

	    const char* source_s = "";
	    switch (source) {
	    case GL_DEBUG_SOURCE_API:
	      source_s = "API";
	      break;
	    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
	      source_s = "Window System";
	      break;
	    case GL_DEBUG_SOURCE_SHADER_COMPILER:
	      source_s = "Shader Compiler";
	      break;
	    case GL_DEBUG_SOURCE_THIRD_PARTY:
	      source_s = "Third Party";
	      break;
	    case GL_DEBUG_SOURCE_APPLICATION:
	      source_s = "Application";
	      break;
	    case GL_DEBUG_SOURCE_OTHER:
	      source_s = "Other";
	      break;
	    default:
	      source_s = "Unknown";
	      break;
	    }
	    fprintf(stderr, "Source: %s\n", source_s);

	    const char* severity_s = "";
	    switch (severity) {
	    case GL_DEBUG_SEVERITY_HIGH:
	      severity_s = "high";
	      break;
	    case GL_DEBUG_SEVERITY_MEDIUM:
	      severity_s = "medium";
	      break;
	    case GL_DEBUG_SEVERITY_LOW:
	      severity_s = "low";
	      break;
	    case GL_DEBUG_SEVERITY_NOTIFICATION:
	      severity_s = "notification";
	      break;
	    default:
	      severity_s = "unknown";
	      break;
	    }
	    fprintf(stderr, "Severity: %s\n\n", severity_s);
	  }
	 , nullptr);
      }
    }

    class ScreenManagerImpl {
    public:
      int width_, height_;
      bool valid_;

      ScreenManagerImpl() noexcept : width_(0), height_(0), valid_(false) {}
      ~ScreenManagerImpl() = default;

      ScreenManagerImpl(const ScreenManagerImpl&) = delete;
      ScreenManagerImpl& operator=(const ScreenManagerImpl&) = delete;
      ScreenManagerImpl(ScreenManagerImpl&&) = delete;
      ScreenManagerImpl& operator=(ScreenManagerImpl&&) = delete;
      
      void init(int, int);
      double aspect() const noexcept { return static_cast<double>(width_) / height_; }
    };

    static ScreenManagerImpl g_scm;
    
    void ScreenManagerImpl::init(int w, int h)
    {
      width_ = w;
      height_ = h;

      glViewport(0, 0, w, h);
  
      valid_ = true;
    }

    void ScreenManager::init(int w, int h)
    {
      assert(!g_scm.valid_);
      g_scm.init(w, h);
    }

    void ScreenManager::reset(int w, int h)
    {
      assert(g_scm.valid_);
      g_scm.init(w, h);
    }
    
    int ScreenManager::width() noexcept
    {
      assert(g_scm.valid_);
      return g_scm.width_;
    }

    int ScreenManager::height() noexcept
    {
      assert(g_scm.valid_);
      return g_scm.height_;
    }

    double ScreenManager::aspectd() noexcept
    {
      assert(g_scm.valid_);
      return g_scm.aspect();
    }

    float ScreenManager::aspectf() noexcept
    {
      assert(g_scm.valid_);
      return static_cast<float>(g_scm.aspect());
    }

    class FramerateCounterImpl {
    public:
      FramerateCounterImpl(unsigned long mrf = 120, float mrms = 2000) noexcept
	: min_report_frames_(mrf), min_report_milliseconds_(mrms),
	  frames_since_report_(0), last_report_time_(SDL_GetTicks()), mean_framerate_(0) {}
      ~FramerateCounterImpl() = default;

      FramerateCounterImpl(const FramerateCounterImpl&) = delete;
      FramerateCounterImpl& operator=(const FramerateCounterImpl&) = delete;
      FramerateCounterImpl(FramerateCounterImpl&&) = delete;
      FramerateCounterImpl& operator=(FramerateCounterImpl&&) = delete;
      
      unsigned long min_report_frames_;
      unsigned long min_report_milliseconds_;
      unsigned long frames_since_report_;
      unsigned long last_report_time_;
      float mean_framerate_;
      unsigned long delta_t_;
    };

    static FramerateCounterImpl g_fc;

    bool FramerateCounter::next_frame()
    {
      g_fc.frames_since_report_++;

      if (g_fc.frames_since_report_ < g_fc.min_report_frames_) {
	return false;
      }

      unsigned long ticks = SDL_GetTicks();
      g_fc.delta_t_ = ticks - g_fc.last_report_time_;
      if (g_fc.delta_t_ > g_fc.min_report_milliseconds_) {
	g_fc.last_report_time_ = ticks;
	g_fc.mean_framerate_= g_fc.frames_since_report_ / (g_fc.delta_t_ / 1000.f);
	g_fc.frames_since_report_ = 0;

	return true;
      }

      return false;
    }

    void FramerateCounter::reset() noexcept
    {
      g_fc.frames_since_report_ = 0;
    }

    float FramerateCounter::mean_framerate() noexcept
    {
      return g_fc.mean_framerate_;
    }

    unsigned long FramerateCounter::delta_t() noexcept
    {
      return g_fc.delta_t_;
    }

    void FramerateCounter::min_report_frames(unsigned long mrf) noexcept
    {
      g_fc.min_report_frames_ = mrf;
    }

    void FramerateCounter::min_report_milliseconds(unsigned long mrms) noexcept
    {
      g_fc.min_report_milliseconds_ = mrms;
    }
  }
}
