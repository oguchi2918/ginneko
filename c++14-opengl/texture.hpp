#ifndef INCLUDED_TEXTURE_HPP
#define INCLUDED_TEXTURE_HPP

#include <glad/glad.h>

namespace nekolib {
  namespace renderer {
    enum class TextureFormat { INVALID = 0,
			       RED8,     // monotone, 0-255
			       RED32F,   // monotone, float
			       RGB8,     // R,G,B each 0-255
			       RGB16F,   // R,G,B each 16bit float
			       RGB32F,   // R,G,B each 32bit float
			       RGBA8,    // R,G,B,A each 0-255
			       RGBA16F,  // R,G,B,A each 16bit float
			       RGBA32F,  // R,G,B,A each 32bit float
			       DEPTH24,  // depth 24bit uint
			       // DEPTH32, // depth 32bit uint
			       DEPTH32F, // depth 32bit float
			       STENCIL8, // stencil 0-255
			       DEPTH24_STENCIL8, // depth 24bit uint + stencil 8bit uint
			       DEPTH32F_STENCIL8,// depth 32bit float + stencil 8bit uint
    };
			      
    class Texture {
    public:
      // 空2Dテクスチャ (RenderTarget(=FBO)等で使用)
      static Texture create(int width, int height, TextureFormat tf);
      // 一枚絵からロードされる2Dテクスチャ
      static Texture create(const char* filename, bool flipped = true);
      // キューブマップを6枚絵からロード
      // filenamesは left, right, top, bottom, back, frontの順番と解釈する
      static Texture create_cubemap(const char** filenames);

      void bind(unsigned) const;
      GLenum type() const noexcept;
      int width() const noexcept;
      int height() const noexcept;
      GLuint handle() const noexcept;
      
      // 以下、参照回数計測クラスのテンプレート関数
      Texture() noexcept;
      ~Texture() noexcept;
      Texture(const Texture&) noexcept;
      Texture(Texture&&) noexcept;
      void release() noexcept;
      Texture& operator=(const Texture&) noexcept;
      Texture& operator=(Texture&&) noexcept;
      bool operator==(const Texture&) const noexcept;
      bool operator!=(const Texture&) const noexcept;
      explicit operator bool() const noexcept;
      
    private:
      class Impl;
      Impl* impl_;
      friend class Manager;
    };
  }
}

#endif // INCLUDED_TEXTURE_HPP
