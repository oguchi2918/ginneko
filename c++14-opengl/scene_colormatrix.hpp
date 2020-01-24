#ifndef INCLUDED_SCENE_COLORMATRIX_HPP
#define INCLUDED_SCENE_COLORMATRIX_HPP

#include <memory>
#include <glm/glm.hpp>

#include "program.hpp"
#include "clock.hpp"
#include "texture.hpp"
#include "globject.hpp"
#include "shape.hpp"

class SceneColorMatrix
{
private:
  nekolib::renderer::Program prog_; // 表示用
  nekolib::renderer::Program cm_prog_; // 色補正 compute shader

  nekolib::renderer::Quad quad_;
  nekolib::renderer::Texture source_tex_;  // immutable
  nekolib::renderer::Texture result_tex_;  // immutable

  bool imgui_ = true;

  void compute();
  bool compile_and_link_shaders();
public:
  SceneColorMatrix() {}
  ~SceneColorMatrix() = default;

  bool init(int* width, int* height);
  void update();
  void render();
};

#endif // INCLUDED_SCENE_COLORMATRIX_HPP
