#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glad/glad.h>

#include "shape.hpp"
#include "base.hpp"
#include "globject.hpp"
#include "defines.hpp"
#include "utils.hpp"

using glm::vec2;
using glm::vec3;

namespace {
  struct vertex_pnt {
    vec3 p;
    vec3 n;
    vec2 t;
  };

  struct vertex_pt {
    vec3 p;
    vec2 t;
  };
}

namespace nekolib {
  namespace renderer {
    // Quad
    // 参照カウンタ持ち実装クラス
    class Quad::Impl : public nekolib::base::Refered {
    private:
      nekolib::renderer::gl::Vao vao_;
      nekolib::renderer::gl::VertexBuffer buffer_;
      nekolib::renderer::gl::IndexBuffer index_;
    public:
      Impl(float umin, float vmin, float umax, float vmax, bool has_norm) {
	struct vertex_pnt vdata_pnt[4] = {
	  {{-1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {umin, vmax}},
	  {{1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {umax, vmax}},
	  {{-1.f, -1.f, 0.f}, {0.f, 0.f, 1.f}, {umin, vmin}},
	  {{1.f, -1.f, 0.f}, {0.f, 0.f, 1.f}, {umax, vmin}},
	};
	struct vertex_pt vdata_pt[4] = {
	  {{-1.f, 1.f, 0.f}, {umin, vmax}},
	  {{1.f, 1.f, 0.f}, {umax, vmax}},
	  {{-1.f, -1.f, 0.f}, {umin, vmin}},
	  {{1.f, -1.f, 0.f}, {umax, vmin}},
	};

	GLuint el[6] = {
	  0, 1, 2, 3,
	};
	
	buffer_.bind();
	if (has_norm) {
	  glBufferData(GL_ARRAY_BUFFER, sizeof(vdata_pnt), vdata_pnt, GL_STATIC_DRAW);
	} else {
	  glBufferData(GL_ARRAY_BUFFER, sizeof(vdata_pt), vdata_pt, GL_STATIC_DRAW);
	}
	index_.bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(el), el, GL_STATIC_DRAW);

	vao_.bind();

	buffer_.bind();
	if (has_norm) {
	  glEnableVertexAttribArray(0); // position
	  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pnt), BUFFER_OFFSET(0));
	  glEnableVertexAttribArray(1); // normal
	  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pnt), BUFFER_OFFSET(sizeof(vec3)));
	  glEnableVertexAttribArray(2); // texture coords
	  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pnt), BUFFER_OFFSET(sizeof(vec3) * 2));
	} else {
	  glEnableVertexAttribArray(0); // position
	  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pt), BUFFER_OFFSET(0));
	  glEnableVertexAttribArray(1); // texture coords
	  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pt), BUFFER_OFFSET(sizeof(vec3)));
	}	  
	index_.bind();
  
	vao_.bind(false);
      }

      ~Impl() = default;

      void render() const {
	vao_.bind();
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
      }
    };
    
    // API実装
    Quad Quad::create(float umin, float vmin, float umax, float vmax, bool has_norm)
    {
      Quad r;
      r.impl_ = new Quad::Impl(umin, vmin, umax, vmax, has_norm);
      return r;
    }
    
    void Quad::render() const
    {
      assert(impl_);
      impl_->render();
    }
#define TYPE Quad
#include "rctype_template.hpp" // auto #undef TYPE
    
    // Cube
    // 参照カウンタ持ち実装クラス
    class Cube::Impl : public nekolib::base::Refered {
    private:
      nekolib::renderer::gl::Vao vao_;
      nekolib::renderer::gl::VertexBuffer buffer_;
      nekolib::renderer::gl::IndexBuffer index_;
    public:
      Impl(bool has_norm = true) {
	float side2 = 1.f / 2.f;

	struct vertex_pnt vdata_pnt[] =
	  {
	   // Front
	   {{-side2, -side2, side2}, {0.f, 0.f, 1.f}, {0.f, 0.f}},
	   {{side2, -side2, side2}, {0.f, 0.f, 1.f}, {1.f, 0.f}},
	   {{side2, side2, side2}, {0.f, 0.f, 1.f}, {1.f, 1.f}},
	   {{-side2, side2, side2}, {0.f, 0.f, 1.f}, {0.f, 1.f}},
	   // Right
	   {{side2, -side2, side2}, {1.f, 0.f, 0.f}, {0.f, 0.f}},
	   {{side2, -side2, -side2}, {1.f, 0.f, 0.f}, {1.f, 0.f}},
	   {{side2, side2, -side2}, {1.f, 0.f, 0.f}, {1.f, 1.f}},
	   {{side2, side2, side2}, {1.f, 0.f, 0.f}, {0.f, 1.f}},
	   // Back
	   {{-side2, -side2, -side2}, {0.f, 0.f, -1.f}, {0.f, 0.f}},
	   {{-side2, side2, -side2}, {0.f, 0.f, -1.f}, {1.f, 0.f}},
	   {{side2, side2, -side2}, {0.f, 0.f, -1.f}, {1.f, 1.f}},
	   {{side2, -side2, -side2}, {0.f, 0.f, -1.f}, {0.f, 1.f}},
	   // Left
	   {{-side2, -side2, side2}, {-1.f, 0.f, 0.f}, {0.f, 0.f}},
	   {{-side2, side2, side2}, {-1.f, 0.f, 0.f}, {1.f, 0.f}},
	   {{-side2, side2, -side2}, {-1.f, 0.f, 0.f}, {1.f, 1.f}},
	   {{-side2, -side2, -side2}, {-1.f, 0.f, 0.f}, {0.f, 1.f}},
	   // Bottom
	   {{-side2, -side2, side2}, {0.f, -1.f, 0.f}, {0.f, 0.f}},
	   {{-side2, -side2, -side2}, {0.f, -1.f, 0.f}, {1.f, 0.f}},
	   {{side2, -side2, -side2}, {0.f, -1.f, 0.f}, {1.f, 1.f}},
	   {{side2, -side2, side2}, {0.f, -1.f, 0.f}, {0.f, 1.f}},
	   // Top
	   {{-side2, side2, side2}, {0.f, 1.f, 0.f}, {0.f, 0.f}},
	   {{side2, side2, side2}, {0.f, 1.f, 0.f}, {1.f, 0.f}},
	   {{side2, side2, -side2}, {0.f, 1.f, 0.f}, {1.f, 1.f}},
	   {{-side2, side2, -side2}, {0.f, 1.f, 0.f}, {0.f, 1.f}},
	  };

	struct vertex_pt vdata_pt[] =
	  {
	   // Front
	   {{-side2, -side2, side2}, {0.f, 0.f}},
	   {{side2, -side2, side2}, {1.f, 0.f}},
	   {{side2, side2, side2}, {1.f, 1.f}},
	   {{-side2, side2, side2}, {0.f, 1.f}},
	   // Right
	   {{side2, -side2, side2}, {0.f, 0.f}},
	   {{side2, -side2, -side2}, {1.f, 0.f}},
	   {{side2, side2, -side2}, {1.f, 1.f}},
	   {{side2, side2, side2}, {0.f, 1.f}},
	   // Back
	   {{-side2, -side2, -side2}, {0.f, 0.f}},
	   {{-side2, side2, -side2}, {1.f, 0.f}},
	   {{side2, side2, -side2}, {1.f, 1.f}},
	   {{side2, -side2, -side2}, {0.f, 1.f}},
	   // Left
	   {{-side2, -side2, side2}, {0.f, 0.f}},
	   {{-side2, side2, side2}, {1.f, 0.f}},
	   {{-side2, side2, -side2}, {1.f, 1.f}},
	   {{-side2, -side2, -side2}, {0.f, 1.f}},
	   // Bottom
	   {{-side2, -side2, side2}, {0.f, 0.f}},
	   {{-side2, -side2, -side2}, {1.f, 0.f}},
	   {{side2, -side2, -side2}, {1.f, 1.f}},
	   {{side2, -side2, side2}, {0.f, 1.f}},
	   // Top
	   {{-side2, side2, side2}, {0.f, 0.f}},
	   {{side2, side2, side2}, {1.f, 0.f}},
	   {{side2, side2, -side2}, {1.f, 1.f}},
	   {{-side2, side2, -side2}, {0.f, 1.f}},
	  };


	GLuint el[] = {
	  0,1,2,0,2,3,
	  4,5,6,4,6,7,
	  8,9,10,8,10,11,
	  12,13,14,12,14,15,
	  16,17,18,16,18,19,
	  20,21,22,20,22,23,
	};

	buffer_.bind();
	if (has_norm) {
	  glBufferData(GL_ARRAY_BUFFER, sizeof(vdata_pnt), vdata_pnt, GL_STATIC_DRAW);
	} else {
	  glBufferData(GL_ARRAY_BUFFER, sizeof(vdata_pt), vdata_pt, GL_STATIC_DRAW);
	}
	index_.bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(el), el, GL_STATIC_DRAW);

	vao_.bind();
	
	buffer_.bind();
	if (has_norm) {
	  glEnableVertexAttribArray(0); // vertex position
	  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pnt), BUFFER_OFFSET(0));
	  glEnableVertexAttribArray(1); // normal
	  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pnt), BUFFER_OFFSET(sizeof(vec3)));
	  glEnableVertexAttribArray(2); // texture coords
	  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pnt), BUFFER_OFFSET(sizeof(vec3) * 2));
	} else {
	  glEnableVertexAttribArray(0); // vertex position
	  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pt), BUFFER_OFFSET(0));
	  glEnableVertexAttribArray(1); // texture coords
	  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pt), BUFFER_OFFSET(sizeof(vec3)));
	}	  
	index_.bind();
  
	vao_.bind(false);
      }
      
      ~Impl() = default;

      void render() const {
	vao_.bind();
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
      }
    };

    // API実装
    Cube Cube::create(bool has_norm)
    {
      static Cube r;
      if (!r.impl_) {
	r.impl_ = new Cube::Impl(has_norm);
      }
      return r;
    }
    
    void Cube::render() const
    {
      assert(impl_);
      impl_->render();
    }
#define TYPE Cube
#include "rctype_template.hpp" // auto #undef TYPE

    // WireCube
    // 参照カウンタ持ち実装クラス
    class WireCube::Impl : public nekolib::base::Refered {
    private:
      nekolib::renderer::gl::Vao vao_;
      nekolib::renderer::gl::VertexBuffer buffer_;
      nekolib::renderer::gl::IndexBuffer index_;
    public:
      Impl() {
	float side2 = 1.f / 2.f;

	vec3 vdata[8] = {
	  {-side2, -side2, -side2},
	  {side2, -side2, -side2},
	  {side2, -side2, side2}, 
	  {-side2, -side2, side2},
	  {-side2, side2, -side2},
	  {side2, side2, -side2},
	  {side2, side2, side2},
	  {-side2, side2, side2},
	};

	GLuint el[24] = {
	  0,1,1,2,2,3,3,0,
	  0,4,1,5,2,6,3,7,
	  4,5,5,6,6,7,7,4

	};

	buffer_.bind();
	glBufferData(GL_ARRAY_BUFFER, sizeof(vdata), vdata, GL_STATIC_DRAW);
	index_.bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(el), el, GL_STATIC_DRAW);

	vao_.bind();

	glEnableVertexAttribArray(0); // vertex position
	buffer_.bind();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	index_.bind();
  
	vao_.bind(false);
      }
      
      ~Impl() = default;

      void render() const noexcept {
	vao_.bind();
	glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
      }
    };

    // API実装
    WireCube WireCube::create()
    {
      static WireCube r;
      if (!r.impl_) {
	r.impl_ = new WireCube::Impl();
      }
      return r;
    }
    void WireCube::render() const
    {
      assert(impl_);
      impl_->render();
    }
#define TYPE WireCube
#include "rctype_template.hpp" // auto #undef TYPE

    // Torus
    // 参照カウンタ持ち実装クラス
    class Torus::Impl : public nekolib::base::Refered {
    private:
      nekolib::renderer::gl::Vao vao_;
      nekolib::renderer::gl::VertexBuffer buffer_;
      nekolib::renderer::gl::IndexBuffer index_;
      const unsigned int faces_, rings_, sides_;
      bool valid_;
    public:
      Impl(float outer_radius, float inner_radius, unsigned sides, unsigned rings)
	: faces_(rings * sides), rings_(rings), sides_(sides), valid_(false) {
	// one extra ring to dupricate first
	int verts_num = (sides_ + 1) * (rings_ + 1);

	struct vertex_pnt* vdata = nullptr;
	GLuint* idata = nullptr;

	// initialize vdata
	vdata = new struct vertex_pnt[verts_num];
	idata = new GLuint[6 * faces_];

	if (vdata == nullptr || idata == nullptr) {
	  return;
	}

	float ring_factor = glm::pi<float>() * 2 / rings_;
	float side_factor = glm::pi<float>() * 2 / sides_;
	unsigned idx = 0;

	for (unsigned ring = 0; ring <= rings_; ++ring) {
	  float u = ring * ring_factor;
	  float cu = cos(u);
	  float su = sin(u);
	  for (unsigned side = 0; side <= sides_; ++side) {
	    float v = side * side_factor;
	    float cv = cos(v);
	    float sv = sin(v);
	    float r = outer_radius - inner_radius * cv;

	    vdata[idx].p = vec3(-r * su, -inner_radius * sv, -r * cu);
	    vdata[idx].n = vec3(cv * su, -sv, cv * cu);
	    vdata[idx].t = vec2(static_cast<float>(ring) / rings_, static_cast<float>(side) / sides_);
	    idx++;
	  }
	}

	// initialize idata
	idx = 0;
	for (unsigned ring = 0; ring < rings_; ++ring) {
	  int ring_start = ring * (sides_ + 1);
	  int next_ring_start = (ring + 1) * (sides_ + 1);
	  for (unsigned side = 0; side < sides_; ++side) {
	    int next_side = side + 1;
	    // quad
	    idata[idx] = ring_start + side;
	    idata[idx + 1] = next_ring_start + side;
	    idata[idx + 2] = next_ring_start + next_side;
	    idata[idx + 3] = ring_start + side;
	    idata[idx + 4] = next_ring_start + next_side;
	    idata[idx + 5] = ring_start + next_side;
	    idx += 6;
	  }
	}

	// populate buffer objects
	buffer_.bind();
	glBufferData(GL_ARRAY_BUFFER, sizeof(struct vertex_pnt) * verts_num, vdata, GL_STATIC_DRAW);
	index_.bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 6 * faces_, idata, GL_STATIC_DRAW);

	// create vao
	vao_.bind();
	
	buffer_.bind();
	glEnableVertexAttribArray(0); // position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pnt), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(1); // normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pnt), BUFFER_OFFSET(sizeof(vec3)));
	glEnableVertexAttribArray(2); // texture coordinate
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pnt), BUFFER_OFFSET(sizeof(vec3) * 2));
	
	index_.bind();
	vao_.bind(false);

	SAFE_DELETE_ARRAY(vdata);
	SAFE_DELETE_ARRAY(idata);
	valid_ = true;
      }

      ~Impl() = default;
      
      bool valid() const noexcept {
	return valid_;
      }

      void render() const {
	vao_.bind();
	glDrawElements(GL_TRIANGLES, 6 * faces_, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
      }
    };

    // API実装
    Torus Torus::create(float outer_radius, float inner_radius, unsigned sides, unsigned rings)
    {
      Torus r;
      r.impl_ = new Torus::Impl(outer_radius, inner_radius, sides, rings);
      if (r.impl_ && !r.impl_->valid()) {
	r.impl_ = nullptr;
      }

      return r;
    }

    void Torus::render() const
    {
      assert(impl_);
      impl_->render();
    }
#define TYPE Torus
#include "rctype_template.hpp" // auto #undef TYPE    

    // Sphere
    // 参照カウンタ持ち実装クラス
    class Sphere::Impl : public nekolib::base::Refered {
    private:
      nekolib::renderer::gl::Vao vao_;
      nekolib::renderer::gl::VertexBuffer buffer_;
      nekolib::renderer::gl::IndexBuffer index_;
      const unsigned faces_, slices_, stacks_;
      bool valid_;
    public:
      Impl(unsigned slices, unsigned stacks) :
	faces_(slices * stacks), slices_(slices), stacks_(stacks), valid_(false) {
	assert(stacks_ % 2 == 0);
	int verts_num = (slices_ + 1) * (stacks_ + 1);

	struct vertex_pnt* vdata = nullptr;
	GLuint* idata = nullptr;

	// initialize vdata
	vdata = new struct vertex_pnt[verts_num];
	idata = new GLuint[6 * faces_];
	if (vdata == nullptr || idata == nullptr) {
	  return;
	}

	float slices_factor = glm::pi<float>() * 2.f / slices_;
	float stacks_factor = glm::pi<float>() / stacks_;
	unsigned idx = 0;

	for (unsigned stack = 0; stack <= stacks_; ++stack) {
	  float v = stack * stacks_factor - glm::pi<float>() / 2.f; // [-pai / 2, pai / 2]
	  float cv = cos(v);
	  float sv = sin(v);
	  for (unsigned slice = 0; slice <= slices_; ++slice) {
	    float u = slice * slices_factor; // [0, pai * 2]
	    float cu = cos(u);
	    float su = sin(u);

	    vdata[idx].p = vdata[idx].n = vec3(-cv * su, sv, -cv * cu);
	    vdata[idx].t = vec2(static_cast<float>(slice) / slices_, static_cast<float>(stack) / stacks_);
	    idx++;
	  }
	}

	// initialize idata
	idx = 0;
	for (unsigned stack = 0; stack < stacks_; ++stack) {
	  int stack_start = stack * (slices_ + 1);
	  int next_stack_start = (stack + 1) * (slices_ + 1);
	  for (unsigned slice = 0; slice < slices_; ++slice) {
	    int next_slice = slice + 1;
	    // quad
	    idata[idx] = stack_start + slice;
	    idata[idx + 1] = next_stack_start + slice;
	    idata[idx + 2] = next_stack_start + next_slice;
	    idata[idx + 3] = stack_start + slice;
	    idata[idx + 4] = next_stack_start + next_slice;
	    idata[idx + 5] = stack_start + next_slice;
	    idx += 6;
	  }
	}

	// populate buffer objects
	buffer_.bind();
	glBufferData(GL_ARRAY_BUFFER, sizeof(struct vertex_pnt) * verts_num, vdata, GL_STATIC_DRAW);
	index_.bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 6 * faces_, idata, GL_STATIC_DRAW);

	// create vao
	vao_.bind();
	
	buffer_.bind();
	glEnableVertexAttribArray(0); // position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pnt), BUFFER_OFFSET(0));
	glEnableVertexAttribArray(1); // normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pnt), BUFFER_OFFSET(sizeof(vec3)));
	glEnableVertexAttribArray(2); // texture coordinate
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex_pnt), BUFFER_OFFSET(sizeof(vec3) * 2));

	index_.bind();
	vao_.bind(false);

	SAFE_DELETE_ARRAY(vdata);
	SAFE_DELETE_ARRAY(idata);
	valid_ = true;
      }

      ~Impl() = default;
	
      bool valid() const noexcept {
	return valid_;
      }

      void render() const {
	vao_.bind();
	glDrawElements(GL_TRIANGLES, 6 * faces_, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
      }
    };

    // API実装
    Sphere Sphere::create(unsigned slices, unsigned stacks)
    {
      Sphere r;
      r.impl_ = new Sphere::Impl(slices, stacks);
      if (r.impl_ && !r.impl_->valid()) {
	r.impl_ = nullptr;
      }
      return r;
    }

    void Sphere::render() const
    {
      assert(impl_);
      impl_->render();
    }
#define TYPE Sphere    
#include "rctype_template.hpp" // auto #undef TYPE

    class WireSphere::Impl : public nekolib::base::Refered {
    private:
      nekolib::renderer::gl::Vao vao_;
      nekolib::renderer::gl::VertexBuffer buffer_;
      nekolib::renderer::gl::IndexBuffer index_;
      const unsigned slices_, stacks_;
      bool valid_;
    public:
      Impl(unsigned slices, unsigned stacks) :
	slices_(slices), stacks_(stacks), valid_(false) {
	assert(stacks_ % 2 == 0);
	int verts_num = slices_ * (stacks_ - 1) + 2;
	int line_num = slices_ * (stacks_ + stacks_ - 1);

	vec3* vdata = nullptr;
	GLuint* idata = nullptr;

	// initialize vdata
	vdata = new vec3[verts_num];
	idata = new GLuint[2 * line_num];

	float slices_factor = glm::pi<float>() * 2.f / slices_;
	float stacks_factor = glm::pi<float>() / stacks_;
	unsigned idx = 0;

	for (unsigned stack = 1; stack < stacks_; stack++) {
	  float v = stack * stacks_factor - glm::pi<float>() / 2.f; // (-pai / 2, pai / 2)
	  float cv = cos(v);
	  float sv = sin(v);
	  for (unsigned slice = 0; slice < slices_; slice++) {
	    float u = slice * slices_factor; // [0, pai * 2)
	    float cu = cos(u);
	    float su = sin(u);

	    vdata[idx++] = vec3(-cv * su, sv, -cv * cu);
	  }
	}
	vdata[idx++] = vec3(0.f, -1.f, 0.f);
	vdata[idx++] = vec3(0.f, 1.f, 0.f);

	// initialize idata
	idx = 0;
	for (unsigned stack = 1; stack < stacks_; stack++) {   // 緯線
	  for (unsigned slice = 0; slice < slices_; slice++) {
	    idata[idx] = (stack - 1) * slices + slice;
	    idata[idx + 1] = (stack - 1) * slices_ + ((slice + 1) % slices_);
	    idx += 2;
	  }
	}
	for (unsigned stack = 1; stack < stacks_ - 1; stack++) { // 経線(極地除)
	  for (unsigned slice = 0; slice < slices_; slice++) {
	    idata[idx] = (stack - 1) * slices_ + slice;
	    idata[idx + 1] = stack * slices_ + slice;
	    idx += 2;
	  }
	}
	for (unsigned slice = 0; slice < slices_; slice++) { // 経線(南極周辺)
	  idata[idx] = verts_num - 2;
	  idata[idx + 1] = slice;
	  idx += 2;
	}
	for (unsigned slice = 0; slice < slices_; slice++) { // 経線(北極周辺)
	  idata[idx] = (stacks_ - 2) * slices_ + slice;
	  idata[idx + 1] = verts_num - 1;
	  idx += 2;
	}

	// populate buffer objects
	buffer_.bind();
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * verts_num, vdata, GL_STATIC_DRAW);
	index_.bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 2 * line_num, idata, GL_STATIC_DRAW);

	// create vao
	vao_.bind();
	
	buffer_.bind();
	glEnableVertexAttribArray(0); // position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), BUFFER_OFFSET(0));

	index_.bind();
	vao_.bind(false);

	SAFE_DELETE_ARRAY(vdata);
	SAFE_DELETE_ARRAY(idata);
	valid_ = true;
      }

      ~Impl() = default;

      bool valid() const noexcept {
	return valid_;
      }

      void render() const {
	vao_.bind();
	glDrawElements(GL_LINES, 2 * slices_ * (stacks_ + stacks_ - 1), GL_UNSIGNED_INT, BUFFER_OFFSET(0));
      }
    };

    // API実装
    WireSphere WireSphere::create(unsigned slices, unsigned stacks)
    {
      WireSphere r;
      r.impl_ = new WireSphere::Impl(slices, stacks);
      if (r.impl_ && !r.impl_->valid()) {
	r.impl_ = nullptr;
      }
      return r;
    }

    void WireSphere::render() const
    {
      assert(impl_);
      impl_->render();
    }
#define TYPE WireSphere    
#include "rctype_template.hpp" // auto #undef TYPE

  }
}
