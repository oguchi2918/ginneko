#ifndef INCLUDED_SCENE_BLOB_HPP
#define INCLUDED_SCENE_BLOB_HPP

#include <vector>
#include <random>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "program.hpp"
#include "clock.hpp"

struct Particle;
class Blob;
using Particles = std::vector<Particle>;

class SceneBlob
{
private:
  nekolib::renderer::Program prog_;
  nekolib::renderer::Program comp_prog_; // コンピュートシェーダ

  nekolib::clock::Clock cur_;
  nekolib::clock::Clock prev_;
  nekolib::clock::Clock start_;

  glm::mat4 model_;
  glm::mat4 view_;
  glm::mat4 proj_;

  glm::quat rot_;   // 現フレームでの回転
  glm::quat orig_;  // マウスドラッグ開始時点での回転
  int drag_start_x_, drag_start_y_;
  
  bool imgui_;
  
  Particles init_data_; // 点群用初期化データ
  std::unique_ptr<Blob> blob_; // 点群
  
  const double reset_interval_ = 5.f; // アニメーションリセット周期
  
  bool compile_and_link_shaders();
  void update_rotation(int, int) noexcept;
  void add_particles(Particles&, int, glm::vec3, float, float, std::mt19937&);
public:
  // ctor, dtor
  // 不完全型へのunique_ptrがメンバ変数にあるのでheader内実装は不可
  SceneBlob();
  ~SceneBlob();

  bool init();
  void update();
  void render();
};

#endif // INCLUDED_SCENE_BLOB_HPP
