#ifndef INCLUDED_SCENE_GOMU2_HPP
#define INCLUDED_SCENE_GOMU2_HPP

#include <memory>
#include <glm/glm.hpp>

#include "program.hpp"
#include "clock.hpp"
#include "texture.hpp"
#include "globject.hpp"
#include "shape.hpp"

struct PixelInfo;
class PointBuffer;

class SceneGomu2
{
private:
  nekolib::renderer::Program point_prog_; // 描画用シェーダー(点)
  nekolib::renderer::Program line_prog_; // 描画用シェーダー(線)
  nekolib::renderer::Program quad_prog_; // 画面大テクスチャ表示シェーダー
  nekolib::renderer::Program comp_prog_; // 力の計算シェーダー
  nekolib::renderer::Program comp_end_prog_; // 両端の節点の力の計算シェーダー

  nekolib::clock::Clock cur_;
  nekolib::clock::Clock prev_;

  nekolib::renderer::Quad quad_;
  nekolib::renderer::Texture render_tex_; // 裏画面
  nekolib::renderer::gl::RenderBuffer pick_; // 3D Pick用

  nekolib::renderer::gl::FrameBuffer fbo_;

  std::unique_ptr<PointBuffer> point_buffer_;
  const unsigned point_num_;

  const float point_size_ = 10.f;
  const unsigned clear_pick_ = static_cast<unsigned>(-1);
  glm::vec4 clear_color_ = glm::vec4(1.f, 1.f, 1.f, 1.f);

  bool imgui_ = false;

  bool compile_and_link_shaders();
public:
  SceneGomu2();
  ~SceneGomu2();

  bool init();
  void update();
  void render();

  bool setup_fbo();
  PixelInfo read_pixel(unsigned, unsigned);
};

#endif // INCLUDED_SCENE_GOMU2_HPP
