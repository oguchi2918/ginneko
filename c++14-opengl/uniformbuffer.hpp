#ifndef INCLUDED_UNIFORMBUFFER_HPP
#define INCLUDED_UNIFORMBUFFER_HPP

#include <cstddef>
#include <cassert>
#include <glad/glad.h>

#include "globject.hpp"

namespace nekolib {
  namespace renderer {
    /* C++側で
       struct T {
         alignas(x) 構造体Tのメンバ
       };
       と宣言して
       shaderからは
       layout (std140) uniform Name
       {
         構造体Tのメンバ
       };
       という形でアクセスされることを想定したUniform Buffer Object
     */
    template <typename T>
    class StructUBO {
    private:
      gl::UniformBuffer ubo_;
      GLsizeiptr blocksize_;

    public:
      StructUBO(const T* data = nullptr, unsigned int count = 1) : ubo_(), blocksize_(0) {
	static GLint alignment = -1;
	if (alignment == -1) {
	  glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
	}
	blocksize_ = (((sizeof(T) - 1) / alignment) + 1) * alignment;
	ubo_.bind();
	glBufferData(GL_UNIFORM_BUFFER, count * blocksize_, nullptr, GL_STATIC_DRAW);
	if (data) {
	  for (size_t i = 0; i < count; ++i) {
	    glBufferSubData(GL_UNIFORM_BUFFER, i * blocksize_, sizeof(T), data + i);
	  }
	}
      }
      ~StructUBO() = default;
	
      StructUBO(const StructUBO&) = delete;
      StructUBO& operator=(const StructUBO&) = delete;
      StructUBO(StructUBO&& rhs) noexcept {
	ubo_ = rhs.ubo_;
	blocksize_ = rhs.blocksize_;
	rhs.blocksize_ = 0;
      }
      StructUBO& operator=(StructUBO&& rhs) noexcept {
	ubo_ = rhs.ubo_;
	blocksize_ = rhs.blocksize_;
	rhs.blocksize_ = 0;
	return *this;
      }
	
      GLuint handle() const noexcept { return ubo_.handle(); }
      void bind() const {
	ubo_.bind();
      }
      void send(const T* data, unsigned int start = 0, unsigned int count = 1) const {
	ubo_.bind();
	for (unsigned int i = 0; i < count; ++i) {
	  glBufferSubData(GL_UNIFORM_BUFFER, (start + i) * blocksize_, sizeof(T), data + i);
	}
      }
      void select(GLuint bp, unsigned int i = 0) const {
	glBindBufferRange(GL_UNIFORM_BUFFER, bp, ubo_.handle(), i * blocksize_, sizeof(T));
      }
    };

    /* Tを組み込み型 or 構造体として
       shader側で
       layout (std140) uniform Name {
         uint size; // 有効要素数
         T data[max_size_];  // 配列本体
       };
       と宣言してアクセスされることを想定したUniform Buffer Object */
    template <typename T, size_t Align = 16>
    class ArrayUBO {
      // AlignはTがdvec3 or dvec4 or それらをメンバに持つ構造体の時のみ32他は16を指定
      static_assert(Align == 16 || Align == 32, "Align must be 16 or 32.\n");
    private:
      gl::UniformBuffer ubo_;
      unsigned int max_size_;
      unsigned int valid_size_;

      // オフセット計算用のダミー構造体
      // C++でalignas(n) T ts[]; と記述した場合先頭要素のみnbyteアラインメントが保証される
      // 一方std140の配列は↓のように個々にalignasされた配列要素が延々並んでいるらしい
      struct dummy__ {
	alignas(4) unsigned int size;
	alignas(Align) T t0;
	alignas(Align) T t1;
      };

      size_t calc_aligned_pos(unsigned int index) const noexcept {
	size_t oso_t0 = offsetof(dummy__, t0);
	size_t oso_t1 = offsetof(dummy__, t1);

	return oso_t0 + (oso_t1 - oso_t0) * index;
      }
    public:
      ArrayUBO(unsigned int maxsize) : ubo_(), max_size_(maxsize), valid_size_(0) {
	ubo_.bind();
	glBufferData(GL_UNIFORM_BUFFER, calc_aligned_pos(maxsize), nullptr, GL_STATIC_DRAW);
      }
      ~ArrayUBO() = default;

      ArrayUBO(const ArrayUBO&) = delete;
      ArrayUBO& operator=(const ArrayUBO&) = delete;
      ArrayUBO(ArrayUBO&& rhs) noexcept {
	ubo_ = rhs.ubo_;
	max_size_ = rhs.max_size_;
	rhs.max_size_ = 0;
	valid_size_ = rhs.valid_size_;
	rhs.valid_size_ = 0;
      }
      ArrayUBO& operator=(ArrayUBO& rhs) noexcept {
	ubo_ = rhs.ubo_;
	max_size_ = rhs.max_size_;
	rhs.max_size_ = 0;
	valid_size_ = rhs.valid_size_;
	rhs.valid_size_ = 0;

	return *this;
      }
	
      GLuint handle() const noexcept { return ubo_.handle(); }
      void bind() const {
	ubo_.bind();
      }
      void send(const T* data, unsigned int count) {
	assert(count <= max_size_);
	valid_size_ = count;

	auto buf_size = calc_aligned_pos(valid_size_);
	char* buf = new char(buf_size);
	memcpy(buf, &count, sizeof(unsigned int));
	for (size_t i = 0; i < count; ++i) {
	  memcpy(buf + calc_aligned_pos(i), data + i, sizeof(T));
	}
	  
	ubo_.bind();
	glBufferSubData(GL_UNIFORM_BUFFER, 0, buf_size, buf);

	delete [] buf;
      }
      void select(GLuint bp) const {
	glBindBufferRange(GL_UNIFORM_BUFFER, bp, ubo_.handle(), 0, calc_aligned_pos(max_size_));
      }
    };
  }
}

#endif // INCLUDED_UNIFORMBUFFER_HPP
