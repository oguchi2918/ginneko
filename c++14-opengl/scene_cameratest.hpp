#ifndef INCLUDED_SCENE_CAMERATEST_HPP
#define INCLUDED_SCENE_CAMERATEST_HPP

#include <glm/glm.hpp>

#include "program.hpp"
#include "texture.hpp"
#include "shape.hpp"
#include "clock.hpp"
#include "camera.hpp"

class SceneCameraTest
{
private:
  nekolib::renderer::Program prog_;
  nekolib::renderer::Texture container_tex_;
  nekolib::renderer::Texture face_tex_;

  nekolib::renderer::Cube cube_;

  nekolib::camera::EulerCamera camera_;
  nekolib::camera::QuatCamera qcamera_;
  
  nekolib::clock::Clock cur_;
  nekolib::clock::Clock prev_;

  glm::mat4 model_;
  glm::mat4 view_;
  glm::mat4 proj_;

  int which_camera_; // 0: QuatCamera, 1: EulerCamera 
  bool imgui_;

  bool compile_and_link_shaders();
public:
  SceneCameraTest() : camera_(glm::vec3(0.f, 0.f, 3.f)), qcamera_(glm::vec3(0.f, 0.f, 3.f)),
		      which_camera_(0), imgui_(false) {}
  ~SceneCameraTest() = default;

  bool init();
  void update();
  void render();
};

#endif // INCLUDED_SCENE_CAMERATEST_HPP
