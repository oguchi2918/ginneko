#ifndef INCLUDED_SCENE_SOLAR2_HPP
#define INCLUDED_SCENE_SOLAR2_HPP

#include <memory>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "program.hpp"
#include "clock.hpp"
#include "texture.hpp"
#include "globject.hpp"
#include "shape.hpp"

struct Point;
class PointsBuffer;
using Points = std::vector<Point>;

// 一定距離を保ち追跡対象と姿勢ベクタを保持するストーカー御用達カメラ
class TrackQuatCamera {
public:
  // ctor and dtor
  TrackQuatCamera(glm::vec3 pos,
		   glm::vec3 target,
		   glm::vec3 up = glm::vec3(0.f, 1.f, 0.f)) noexcept;
  ~TrackQuatCamera() = default;

  void update() noexcept;

  void set_target(glm::vec3) noexcept; // 追跡対象を再設定
  glm::mat4 view_matrix() const noexcept;
private:
  // position, rotation, distance
  glm::vec3 target_; // camera target position
  glm::quat cq_; // camera rotation
  const float distance_; // distance between camera and target

  // delta
  glm::quat dq_ = glm::quat(1.f, 0.f, 0.f, 0.f); // camera rotation delta

  const glm::vec3 org_front_;
  const glm::vec3 org_up_;

  glm::vec3 front_;
  glm::vec3 up_;

  // drag data
  bool drag_ = false; // left button dragging
  int drag_sx_ = 0; // drag start mouse x
  int drag_sy_ = 0; // drag start mouse y

  glm::vec3 calc_up(glm::vec3, glm::vec3) noexcept;
  void reset_delta() noexcept;
  void update_dirs() noexcept;
  void process_drag(int dx, int dy) noexcept;
};

class SceneSolar2
{
private:
  // 描画シェーダー
  nekolib::renderer::Program points_prog_; // 点描画
  nekolib::renderer::Program quad_prog_; // 画面大テクスチャ表示
  nekolib::renderer::Program locus_prog_; // 軌跡表示用
  nekolib::renderer::Program axis_prog_; // XYZ軸描画

  // 計算シェーダー
  // Velocity Verlet法
  nekolib::renderer::Program vver_init_prog_;
  nekolib::renderer::Program vver_prog_;
  
  // 座標軸描画
  nekolib::renderer::gl::Vao axis_vao_;
  nekolib::renderer::gl::VertexBuffer axis_vbo_;

  TrackQuatCamera camera_;
  glm::mat4 model_;
  glm::mat4 view_;
  glm::mat4 proj_;
  
  nekolib::clock::Clock cur_;
  nekolib::clock::Clock prev_;

  nekolib::renderer::Quad quad_;

  nekolib::renderer::Texture render_tex_[2];
  nekolib::renderer::gl::FrameBuffer fbo_[2]; // 裏画面
  int current_; // 描画に使用する裏画面のindex

  std::unique_ptr<PointsBuffer> points_buffer_;
  unsigned point_num_;

  bool locus_;
  bool pause_;
  bool imgui_;

  void render_axis(glm::vec3, glm::vec3);
  bool compile_and_link_shaders();
  Points generate_init_data(size_t, unsigned, float, float);
public:
  SceneSolar2(unsigned);
  ~SceneSolar2();

  bool init();
  void update();
  void render();

  bool setup_fbo();
};

#endif // INCLUDED_SCENE_SOLAR2_HPP
