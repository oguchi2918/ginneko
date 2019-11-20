#ifndef INCLUDED_SCENE_GOMU_HPP
#define INCLUDED_SCENE_GOMU_HPP

#include <memory>
#include <glm/glm.hpp>

#include "program.hpp"
#include "clock.hpp"

class PointBuffer;

class SceneGomu
{
private:
  nekolib::renderer::Program prog_;
  nekolib::renderer::Program force_prog_;

  std::unique_ptr<PointBuffer> point_buffer_;

  const float point_size_ = 10.f;

  bool imgui_;

  bool compile_and_link_shaders();
public:
  SceneGomu();
  ~SceneGomu();

  bool init();
  void update();
  void render();
};

#endif // INCLUDED_SCENE_GOMU_HPP
