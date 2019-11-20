#include <cassert>
#include <tuple>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "texture.hpp"
#include "base.hpp"
#include "globject.hpp"
#include "utils.hpp"
#include "defines.hpp"

namespace nekolib {
  namespace renderer {

    // 参照カウンタ持ち実装クラス
    class Texture::Impl : public nekolib::base::Refered {
    public:
      GLenum type_;
      int w_, h_;
      gl::Texture texture_;
    public:
      // 空2Dテクスチャ
      // RenderTarget(=FBO)で書き込んで後にシェーダーでsampler2Dで読むのが定跡
      Impl(int w, int h, TextureFormat tf)
	: type_(0), w_(0), h_(0), texture_() {
	texture_.bind(0, GL_TEXTURE_2D);
	GLenum table[][3] =
	  {
	   { 0, 0, 0 },
	   { GL_R8, GL_RED, GL_UNSIGNED_BYTE} ,
	   { GL_R32F, GL_RED, GL_FLOAT },
	   { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE },
	   { GL_RGB16F, GL_RGB, GL_FLOAT },
	   { GL_RGB32F, GL_RGB, GL_FLOAT },
	   { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE },
	   { GL_RGBA16F, GL_RGBA, GL_FLOAT },
	   { GL_RGBA32F, GL_RGBA, GL_FLOAT },
	   { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT },
	   //{ GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT },
	   { GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT },
	   { GL_STENCIL_INDEX8, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE },
	   { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8 },
	   { GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV },
	  };
	
	if (tf != TextureFormat::INVALID) {
	  size_t index = static_cast<size_t>(tf);
	  glTexImage2D(GL_TEXTURE_2D, 0, table[index][0], w, h, 0, table[index][1], table[index][2], nullptr);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	  
	  // テクスチャ生成に成功してから情報を保持させる
	  if (!check_gl_error(__FILE__, __LINE__)) {
	    type_ = GL_TEXTURE_2D;
	    w_ = w; h_ = h;
	  }
	}

      }
      
      // 一枚絵2Dテクスチャをファイルから読みこみ
      Impl(const char* filename, bool flipped)
	: type_(0), w_(0), h_(0), texture_() {
	if (flipped) {
	  stbi_set_flip_vertically_on_load(true);
	}
	int width, height, components;
	unsigned char* data = stbi_load(filename, &width, &height, &components, 0);
	if (!data) {
	  fprintf(stderr, "stbi_load error.\n");
	  return;
	}
	stbi_set_flip_vertically_on_load(false);

	GLenum iformat, format, wrap;
	switch (components) {
	case 1:
	  iformat = GL_R8;
	  format = GL_RED;
	  wrap = GL_REPEAT;
	  break;
	case 3:
	  iformat = GL_RGB8;
	  format = GL_RGB;
	  wrap = GL_REPEAT;
	  break;
	case 4:
	  iformat = GL_RGBA8;
	  format = GL_RGBA;
	  wrap = GL_CLAMP_TO_EDGE; // GL_REPEAT is not suited for transparent texture.
	  break;
	default:
	  assert(!"Texture class doesn't support this format, sorry.\n");
	  return;
	  break;
	}
	texture_.bind(0, GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, iformat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	
	stbi_image_free(data);
	
	if (!check_gl_error(__FILE__, __LINE__)) {
	  type_ = GL_TEXTURE_2D;
	  w_ = width; h_ = height;
	}
      }
      // キューブマップを同サイズ6枚絵から作成
      // 画像サイズはあまり使わないしチェック緩い
      Impl(const char** cubemap_filenames)
	: type_(0), w_(0), h_(0), texture_() {
	texture_.bind(0, GL_TEXTURE_CUBE_MAP);

	GLuint targets[] = {
	  GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	  GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	  GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	};

	int width, height, components;
	GLenum format(0);
	for (int i = 0; i < 6; ++i) {
	  unsigned char* data = stbi_load(cubemap_filenames[i], &width, &height, &components, 0);
	  if (!data || width != height) {
	    return;
	  }
	  switch (components) {
	  case 1:
	    format = GL_RED;
	    break;
	  case 3:
	    format = GL_RGB;
	    break;
	  case 4:
	    format = GL_RGBA;
	    break;
	  default:
	    assert(!"Texture class doesn't support this format, sorry.\n");
	    break;
	  }
	  glTexImage2D(targets[i], 0, format, width, height, 0,
		       format, GL_UNSIGNED_BYTE, data);
	  stbi_image_free(data);
	}
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	if (!check_gl_error(__FILE__, __LINE__)) {
	  type_ = GL_TEXTURE_CUBE_MAP;
	  w_ = width; h_ = height;
	}
      }

      ~Impl() = default;

      void bind(unsigned no) const {
	texture_.bind(no, type_);
      }

      bool valid() const noexcept { return type_ != 0; }
      GLuint handle() const noexcept { return texture_.handle(); }
    };

    // 以下公開API実装
    Texture Texture::create(int w, int h, TextureFormat tf)
    {
      Texture r;
      r.impl_ = new Texture::Impl(w, h, tf);
      if (!r.impl_->valid()) {
	SAFE_DELETE(r.impl_);
      }
      return r;
    }

    Texture Texture::create(const char* filename, bool flipped)
    {
      Texture r;
      r.impl_ = new Texture::Impl(filename, flipped);
      if (!r.impl_->valid()) {
	SAFE_DELETE(r.impl_);
      }
      return r;
    }

    Texture Texture::create_cubemap(const char** filenames)
    {
      Texture r;
      r.impl_ = new Texture::Impl(filenames);
      if (!r.impl_->valid()) {
	SAFE_DELETE(r.impl_);
      }
      return r;
    }
    
    void Texture::bind(unsigned no) const
    {
      assert(impl_);
      impl_->bind(no);
    }

    GLenum Texture::type() const noexcept
    {
      assert(impl_);
      return impl_->type_;
    }

    int Texture::width() const noexcept
    {
      assert(impl_);
      return impl_->w_;
    }

    int Texture::height() const noexcept
    {
      assert(impl_);
      return impl_->h_;
    }

    GLuint Texture::handle() const noexcept
    {      
      assert(impl_);
      return impl_->handle();
    }

#define TYPE Texture
#include "rctype_template.hpp" // auto #undef TYPE
  }
}
