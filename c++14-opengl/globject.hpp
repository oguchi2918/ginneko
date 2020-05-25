#ifndef INCLUDED_GLOBJECT_HPP
#define INCLUDED_GLOBJECT_HPP

#include <cstddef>
#include <cassert>
#include <glad/glad.h>

namespace nekolib {
  namespace renderer {
    namespace gl {
      // OpenGLの各種Objectを薄くラップした具象クラスなので継承禁止
      // 直接参照回数計測はしない
      // RAII, copy禁止, move可能

      // テクスチャ
      class Texture {
      private:
	GLuint handle_;
      public:
	Texture() : handle_(0) {
	  glGenTextures(1, &handle_);
	}
	~Texture() {
	  glDeleteTextures(1, &handle_);
	}

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture(Texture&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	}
	Texture& operator=(Texture&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	  return *this;
	}

	GLuint handle() const noexcept { return handle_; }
	void bind(unsigned no, int texture_type) const {
	  glActiveTexture(GL_TEXTURE0 + no);
	  glBindTexture(texture_type, handle_);
	}
      };

      // Vertex Array Object
      class Vao {
      private:
	GLuint handle_;
      public:
	Vao() : handle_(0) {
	  glGenVertexArrays(1, &handle_);
	}
	~Vao() noexcept {
	  glDeleteVertexArrays(1, &handle_);
	}

	Vao(const Vao&) = delete;
	Vao& operator=(const Vao&) = delete;
	Vao(Vao&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	}
	Vao& operator=(Vao&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	  return *this;
	}


	GLuint handle() const noexcept { return handle_; }
	void bind(bool flag = true) const {
	  glBindVertexArray(flag ? handle_ : 0);
	}
      };

      // 頂点配列
      class VertexBuffer {
      private:
	GLuint handle_;
      public:
	VertexBuffer() : handle_(0) {
	  glGenBuffers(1, &handle_);
	}
	~VertexBuffer() noexcept {
	  glDeleteBuffers(1, &handle_);
	}

	VertexBuffer(const VertexBuffer&) = delete;
	VertexBuffer& operator=(const VertexBuffer&) = delete;
	VertexBuffer(VertexBuffer&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	}
	VertexBuffer& operator=(VertexBuffer&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	  return *this;
	}

	GLuint handle() const noexcept { return handle_; }
	void bind() const {
	  glBindBuffer(GL_ARRAY_BUFFER, handle_);
	}
      };

      // インデックス配列
      class IndexBuffer {
      private:
	GLuint handle_;
      public:
	IndexBuffer() : handle_(0) {
	  glGenBuffers(1, &handle_);
	}
	~IndexBuffer() noexcept {
	  glDeleteBuffers(1, &handle_);
	}

	IndexBuffer(const IndexBuffer&) = delete;
	IndexBuffer& operator=(const IndexBuffer&) = delete;
	IndexBuffer(IndexBuffer&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	}
	IndexBuffer& operator=(IndexBuffer&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	  return *this;
	}
	
	GLuint handle() const noexcept { return handle_; }
	void bind() const {
	  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle_);
	}
      };

      // テクスチャバッファ
      class TexBuffer {
      private:
	GLuint buffer_handle_;
	GLuint tex_handle_;
	GLenum iformat_;
      public:
	TexBuffer(GLenum iformat) : buffer_handle_(0), tex_handle_(0), iformat_(iformat) {
	  glGenBuffers(1, &buffer_handle_);
	  glGenTextures(1, &tex_handle_);
	  glBindBuffer(GL_TEXTURE_BUFFER, buffer_handle_);
	  glActiveTexture(GL_TEXTURE0); // no need?
	  glBindTexture(GL_TEXTURE_BUFFER, tex_handle_);
	  glTexBuffer(GL_TEXTURE_BUFFER, iformat, buffer_handle_);
	}
	~TexBuffer() noexcept {
	  glDeleteTextures(1, &tex_handle_);
	  glDeleteBuffers(1, &buffer_handle_);
	}

	TexBuffer(const TexBuffer&) = delete;
	TexBuffer& operator=(const TexBuffer&) = delete;
	TexBuffer(TexBuffer&& rhs) noexcept {
	  buffer_handle_ = rhs.buffer_handle_;
	  tex_handle_ = rhs.tex_handle_;
	  rhs.buffer_handle_ = rhs.tex_handle_ = 0;
	}
	TexBuffer& operator=(TexBuffer&& rhs) noexcept {
	  buffer_handle_ = rhs.buffer_handle_;
	  tex_handle_ = rhs.tex_handle_;
	  rhs.buffer_handle_ = rhs.tex_handle_ = 0;
	  return *this;
	}
	
	GLuint handle() const noexcept { return buffer_handle_; }
	GLenum iformat() const noexcept { return iformat_; }
	void bind() const {
	  glBindBuffer(GL_TEXTURE_BUFFER, buffer_handle_);
	}
	void bind_tex(unsigned no) const {
	  glActiveTexture(GL_TEXTURE0 + no);
	  glBindTexture(GL_TEXTURE_BUFFER, tex_handle_);
	}
      };
      
      // フレームバッファ
      class FrameBuffer {
      private:
	GLuint handle_;
      public:
	FrameBuffer() : handle_(0) {
	  glGenFramebuffers(1, &handle_);
	}
	~FrameBuffer() noexcept {
	  glDeleteFramebuffers(1, &handle_);
	}

	FrameBuffer(const FrameBuffer&) = delete;
	FrameBuffer& operator=(const FrameBuffer&) = delete;
	FrameBuffer(FrameBuffer&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	}
	FrameBuffer& operator=(FrameBuffer&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	  return *this;
	}
	
	GLuint handle() const noexcept { return handle_; }
	void bind(bool flag = true) const {
	  glBindFramebuffer(GL_FRAMEBUFFER, flag ? handle_ : 0);
	}
      };

      // レンダーバッファ
      class RenderBuffer {
      private:
	GLuint handle_;
      public:
	struct Desc {
	  GLenum format;
	  int width;
	  int height;
	};
	RenderBuffer() : handle_(0) {
	  glGenRenderbuffers(1, &handle_);
	}
	~RenderBuffer() noexcept {
	  glDeleteRenderbuffers(1, &handle_);
	}

	RenderBuffer(const RenderBuffer&) = delete;
	RenderBuffer& operator=(const RenderBuffer&) = delete;
	RenderBuffer(RenderBuffer&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	}
	RenderBuffer& operator=(RenderBuffer&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	  return *this;
	}
	
	GLuint handle() const noexcept { return handle_; }
	void bind() const {
	  glBindRenderbuffer(GL_RENDERBUFFER, handle_);
	}
      };

      class UniformBuffer {
      private:
	GLuint handle_;
      public:
	UniformBuffer() : handle_(0) {
	  glGenBuffers(1, &handle_);
	}
	~UniformBuffer() {
	  glDeleteBuffers(1, &handle_);	  
	}

	// copy NG, move OK
	UniformBuffer(const UniformBuffer&) = delete;
	UniformBuffer& operator=(const UniformBuffer&) = delete;
	UniformBuffer(UniformBuffer&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	}
	UniformBuffer& operator=(UniformBuffer&& rhs) noexcept {
	  handle_ = rhs.handle_;
	  rhs.handle_ = 0;
	  return *this;
	}
	
	GLuint handle() const noexcept { return handle_; }
	void bind() const {
	  glBindBuffer(GL_UNIFORM_BUFFER, handle_);
	}
      };

    }
  }
}

#endif // INCLUDED_GLOBJECT_HPP
