#include <glad/glad.h>

#include <cstdio>
#include <cmath>
#include <SDL2/SDL_image.h>

#include "utils.hpp"

namespace nekolib {
  int wrap(int x, int low, int high)
  {
    assert(low < high);
    const int n = (x - low) % (high - low);
    return (n >= 0) ? (n + low) : (n + high);
  }

  float wrap(float x, float low, float high)
  {
    assert(low < high);
    const float n = std::fmod(x - low, high - low);
    return (n >= 0) ? (n + low) : (n + high);
  }
  
  namespace renderer {
    bool check_gl_error(const char* file, int line)
    {
      // return 1 if OpenGL error occurred, 0 otherwise.
      GLenum error;
      bool ret = false;

      while ((error  = glGetError()) != GL_NO_ERROR) {
	const char* error_s = "";
	switch (error) {
	case GL_INVALID_ENUM:
	  error_s = "INVALID_ENUM";
	  break;
	case GL_INVALID_VALUE:
	  error_s = "INVALID_VALUE";
	  break;
	case GL_INVALID_OPERATION:
	  error_s = "INVALID_OPERATION";
	  break;
	case GL_STACK_OVERFLOW:
	  error_s = "STACK_OVERFLOW";
	  break;
	case GL_STACK_UNDERFLOW:
	  error_s = "STACK_UNDERFLOW";
	  break;
	case GL_OUT_OF_MEMORY:
	  error_s = "OUT_OF_MEMORY";
	  break;
	case GL_INVALID_FRAMEBUFFER_OPERATION:
	  error_s = "INVALID_FRAMEBUFFER_OPERATION";
	  break;
	default:
	  error_s = "OTHER ERROR";
	  break;
	}
	fprintf(stderr, "OpenGL error in file %s @line %d: %s\n", file, line, error_s);
	ret = true;
      }

      return ret;
    }

    bool check_fbo_status(const char* file, int line)
    {
      GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      switch (status) {
      case GL_FRAMEBUFFER_COMPLETE:
	return false;
	break;
      case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
	fprintf(stderr, "OpenGL can't create FBO in file %s @line %d: ", file, line);
	fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS error.\n");
	break;
      case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
	fprintf(stderr, "OpenGL can't create FBO in file %s @line %d: ", file, line);
	fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT error.\n");
	break;
      case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
	fprintf(stderr, "OpenGL can't create FBO in file %s @line %d: ", file, line);
	fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT error.\n");
	break;
      case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
	fprintf(stderr, "OpenGL can't create FBO in file %s @line %d: ", file, line);
	fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER error.\n");
	break;
      case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
	fprintf(stderr, "OpenGL can't create FBO in file %s @line %d: ", file, line);
	fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER error.\n");
	break;
      case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
	fprintf(stderr, "OpenGL can't create FBO in file %s @line %d: ", file, line);
	fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE error.\n");
	break;
      case GL_FRAMEBUFFER_UNSUPPORTED:
	fprintf(stderr, "OpenGL can't create FBO in file %s @line %d: ", file, line);
	fprintf(stderr, "GL_FRAMEBUFFER_UNSUPPORTED error.\n");
	break;
      default:
	fprintf(stderr, "OpenGL can't create FBO in file %s @line %d: ", file, line);
	fprintf(stderr, "unknown error.\n");
	break;
      }

      return true;
    }

    void dump_gl_info(bool dump_extensions)
    {
      const GLubyte* renderer = glGetString(GL_RENDERER);
      const GLubyte* vendor = glGetString(GL_VENDOR);
      const GLubyte* version = glGetString(GL_VERSION);
      const GLubyte* glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);

      GLint major, minor;
      glGetIntegerv(GL_MAJOR_VERSION, &major);
      glGetIntegerv(GL_MINOR_VERSION, &minor);

      printf("GL Vendor     : %s\n", vendor);
      printf("GL Renderer   : %s\n", renderer);
      printf("GL Version    : %s\n", version);
      printf("GL Version    : %d.%d\n", major, minor);
      printf("GLSL Version  : %s\n", glsl_version);

      if (dump_extensions) {
	GLint extensions_num;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_num);
	for (int i = 0; i < extensions_num; ++i) {
	  printf("%s\n", glGetStringi(GL_EXTENSIONS, i));
	}
      }
    }

  }
}
