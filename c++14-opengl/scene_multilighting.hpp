#ifndef INCLUDED_SCENE_MULTILIGHTING_HPP
#define INCLUDED_SCENE_MULTILIGHTING_HPP

#include <glm/glm.hpp>

#include "program.hpp"
#include "texture.hpp"
#include "shape.hpp"
#include "clock.hpp"
#include "camera.hpp"

class SceneMultiLighting
{
private:
  nekolib::renderer::Program prog_;
  nekolib::renderer::Program lamp_prog_;
  nekolib::renderer::Texture diffuse_map_;
  nekolib::renderer::Texture specular_map_;

  nekolib::renderer::Cube cube_;

  nekolib::camera::EulerCamera camera_;
  
  nekolib::clock::Clock cur_;
  nekolib::clock::Clock prev_;

  glm::mat4 model_;
  glm::mat4 view_;
  glm::mat4 proj_;

  static const int point_lights_num_ = 4;

  bool imgui_;

  glm::vec3 calc_attenuation_parm(int);
  bool compile_and_link_shaders();
public:
  SceneMultiLighting() : camera_(glm::vec3(0.f, 0.f, 3.f)), imgui_(false) {}
  ~SceneMultiLighting() = default;

  bool init();
  void update();
  void render();
};

#endif // INCLUDED_SCENE_MULTILIGHTING_HPP
