#ifndef INCLUDED_SCENE_GOMU2_HPP
#define INCLUDED_SCENE_GOMU2_HPP

#include <memory>
#include <glm/glm.hpp>

#include "program.hpp"
#include "clock.hpp"

class PointBuffer;

class SceneGomu2
{
private:
  nekolib::renderer::Program prog_; // 描画用シェーダー
  nekolib::renderer::Program comp_prog_; // 力の計算シェーダー
  nekolib::renderer::Program comp_end_prog_; // 両端の節点の力の計算シェーダー

  nekolib::clock::Clock cur_;
  nekolib::clock::Clock prev_;

  std::unique_ptr<PointBuffer> point_buffer_;
  const unsigned point_num_;

  const float point_size_ = 10.f;

  bool imgui_ = false;

  bool compile_and_link_shaders();
public:
  SceneGomu2();
  ~SceneGomu2();

  bool init();
  void update();
  void render();
};

#endif // INCLUDED_SCENE_GOMU2_HPP
