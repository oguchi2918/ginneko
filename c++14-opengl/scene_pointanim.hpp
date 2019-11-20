#ifndef INCLUDED_SCENE_POINTANIM_HPP
#define INCLUDED_SCENE_POINTANIM_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "program.hpp"
#include "clock.hpp"
#include "globject.hpp"

class ScenePointAnim
{
private:
  nekolib::renderer::Program prog_;
  nekolib::renderer::gl::Vao vao_;
  nekolib::renderer::gl::VertexBuffer buffer_;

  nekolib::clock::Clock cur_;
  nekolib::clock::Clock start_;

  glm::mat4 model_;
  glm::mat4 view_;
  glm::mat4 proj_;

  glm::quat rot_;   // 現フレームでの回転
  glm::quat orig_;  // マウスドラッグ開始時点での回転
  int drag_start_x_, drag_start_y_;
  
  bool imgui_;

  bool compile_and_link_shaders();
  void update_rotation(int, int);
  void disseminate(size_t);
public:
  ScenePointAnim() : rot_(1.f, 0.f, 0.f, 0.f), orig_(1.f, 0.f, 0.f, 0.f),
		     drag_start_x_(0), drag_start_y_(0), imgui_(false) {}
  ~ScenePointAnim() = default;

  bool init();
  void update();
  void render();
};

#endif // INCLUDED_SCENE_POINTANIM_HPP
