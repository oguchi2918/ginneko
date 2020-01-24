#ifndef INCLUDED_SCENE_IMAGEPROCESS_HPP
#define INCLUDED_SCENE_IMAGEPROCESS_HPP

#include <memory>
#include <glm/glm.hpp>

#include "program.hpp"
#include "clock.hpp"
#include "texture.hpp"
#include "globject.hpp"
#include "shape.hpp"

class SceneImageProcess
{
private:
  nekolib::renderer::Program prog_; // 表示用
  nekolib::renderer::Program invert_prog_; // フィルタ用 compute shader その1
  nekolib::renderer::Program mean_prog_; // フィルタ用 compute shader その2
  nekolib::renderer::Program laplacian_prog_; // フィルタ用 compute shader その3
  nekolib::renderer::Program sobel_prog_; // フィルタ用 compute shader その4

  nekolib::renderer::Quad quad_;
  nekolib::renderer::Texture source_tex_;  // immutable
  nekolib::renderer::Texture result_tex_;  // immutable

  bool imgui_ = true;

  void filter(nekolib::renderer::Program&);
  bool compile_and_link_shaders();
public:
  SceneImageProcess() {}
  ~SceneImageProcess() = default;

  bool init(int* width, int* height);
  void update();
  void render();
};

#endif // INCLUDED_SCENE_IMAGEPROCESS_HPP
