#ifndef INCLUDED_SCENE_SOLAR_HPP
#define INCLUDED_SCENE_SOLAR_HPP

#include <memory>
#include <glm/glm.hpp>

#include "program.hpp"
#include "clock.hpp"
#include "texture.hpp"
#include "globject.hpp"
#include "shape.hpp"

struct Point;
class PointsBuffer;
using Points = std::vector<Point>;

class SceneSolar
{
private:
  // 描画シェーダー
  nekolib::renderer::Program points_prog_; // 点描画シェーダー
  nekolib::renderer::Program quad_prog_; // 画面大テクスチャ表示シェーダー

  // 計算シェーダー
  // Runge Kutta法
  nekolib::renderer::Program rk_init_prog_;
  nekolib::renderer::Program rk_prog_;
  nekolib::renderer::Program rk_finish_prog_;
  // Velocity Verlet法
  nekolib::renderer::Program vver_init_prog_;
  nekolib::renderer::Program vver_prog_;
  
  nekolib::clock::Clock cur_;
  nekolib::clock::Clock prev_;

  nekolib::renderer::Quad quad_;
  
  nekolib::renderer::Texture render_tex_[2];
  nekolib::renderer::gl::FrameBuffer fbo_[2]; // 裏画面
  int current_; // 描画に使用する裏画面のindex

  std::unique_ptr<PointsBuffer> points_buffer_;

  bool locus_;
  bool pause_;
  bool cme_; // calc_moments_and_energy
  bool imgui_;

  bool compile_and_link_shaders();
  void correct_init_data(Points&);
  Points generate_random_init_data(size_t, unsigned, float, float);
public:
  SceneSolar();
  ~SceneSolar();

  bool init();
  void update();
  void render();

  bool setup_fbo();
};

#endif // INCLUDED_SCENE_SOLAR_HPP
