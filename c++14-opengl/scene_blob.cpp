#include <vector>
#include <random>
#include <memory>
#include <cstdio>
#include <cmath>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"

#include "scene_blob.hpp"
#include "defines.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "utils.hpp"
#include "globject.hpp"

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

using namespace nekolib::renderer;

// 粒子データ
struct Particle
{
  glm::vec4 position; // 位置
  glm::vec4 velocity; // 速度

  Particle() noexcept {}
  Particle(float px, float py, float pz, float vx = 0.f, float vy = 0.f, float vz = 0.f) noexcept
    : position(px, py, pz, 1.f), velocity(vx, vy, vz, 0.f) {}
  ~Particle() noexcept {}
};

// 点群
class Blob
{
private:
  nekolib::renderer::gl::Vao vao_;
  nekolib::renderer::gl::VertexBuffer vbo_;

  const GLsizei count_; // 頂点数
public:
  // ctor & dtor
  Blob(const Particles& particles);
  virtual ~Blob();

  // copy
  Blob(const Blob&) = delete;
  Blob& operator=(const Blob&) = delete;

  void init(const Particles& particles) const;
  void render() const; // 描画 
  void update() const; // 位置と速度の更新
};

// particles : in 初期化データ
Blob::Blob(const Particles& particles) : count_(static_cast<GLsizei>(particles.size()))
{
  vao_.bind();
  vbo_.bind();
  glBufferData(GL_ARRAY_BUFFER, count_ * sizeof(Particle), nullptr, GL_STATIC_DRAW);

  // シェーダin変数割り当て
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), BUFFER_OFFSETOF(Particle, position));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), BUFFER_OFFSETOF(Particle, velocity));
  glEnableVertexAttribArray(1);

  init(particles);
}

Blob::~Blob() = default;

void Blob::init(const Particles& particles) const
{
  vbo_.bind();

  // 頂点バッファにデータを格納
  Particle* p = static_cast<Particle*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

  for (const auto& particle : particles) {
    p->position = particle.position;
    p->velocity = particle.velocity;
    ++p;
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);
}

void Blob::render() const
{
  vao_.bind();
  glDrawArrays(GL_POINTS, 0, count_);
}

// コンピュートシェーダーを使って頂点バッファ上の位置/速度を更新
void Blob::update() const
{
  // SSBO0番の結合ポイントとして頂点バッファをそのまま使用
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_.handle());

  // 計算実行
  glDispatchCompute(count_, 1, 1);

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

SceneBlob::SceneBlob() : rot_(1.f, 0.f, 0.f, 0.f), orig_(1.f, 0.f, 0.f, 0.f),
			 drag_start_x_(0), drag_start_y_(0), imgui_(false)
{
}

SceneBlob::~SceneBlob() = default;

bool SceneBlob::init()
{
  if (!compile_and_link_shaders()) {
    return false;
  }

  cur_ = nekolib::clock::Clock::create(0.f);
  prev_ = cur_.snapshot();
  start_ = cur_.snapshot();
  
  view_ = glm::lookAt(vec3(0.f, 0.f, 5.f), vec3(0.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f));
  proj_ = glm::perspective(glm::radians(30.f), ScreenManager::aspectf(), 1.f, 10.f);

  // 粒子群生成用パラメーター
  const int blob_count(8);
  const float blob_range(1.5f);
  const int particle_count(1000);
  const float particle_mean(0.f);
  const float particle_deviation(1.f);
  
  std::random_device seed;
  std::mt19937 rn(seed());
  std::uniform_real_distribution<float> center(-blob_range, blob_range);

  for (auto i = 0; i < blob_count; ++i) {
    // Particle中心位置
    const vec3 pos(center(rn), center(rn), center(rn));
    add_particles(init_data_, particle_count, pos, particle_mean, particle_deviation, rn);
  }

  blob_ = std::make_unique<Blob>(init_data_);
  
  prog_.use();

  glClearColor(0.1f, 0.2f, 0.3f, 0.f);

  fprintf(stdout, "You can drag left mouse button to rotate entire scene.\n");
  fprintf(stdout, "Press 'd' key to show color config dialog.\n");
  
  return true;
}

void SceneBlob::update_rotation(int x, int y) noexcept
{
  float dx = static_cast<float>(x - drag_start_x_) / ScreenManager::width();
  float dy = static_cast<float>(y - drag_start_y_) / ScreenManager::height();
  
  float a = sqrt(dx * dx + dy * dy);
  if (a != 0.f) {
    float ar = a * glm::pi<float>();
    glm::quat dr = glm::rotate(glm::quat(1, 0, 0, 0), 2 * ar, vec3(dy, dx, 0.f));
    rot_ = dr * orig_;
  }
}

// paticles 粒子群の追加格納先
// count 粒子群の粒子数
// center 粒子群の中心位置
// mean 粒子の粒子群の中心からの距離の平均値
// deviation 粒子の粒子群の中心からの距離の標準偏差
// rn メルセンヌツイスタ法による乱数生成器
void SceneBlob::add_particles(Particles& particles, int count,
			      glm::vec3 center, float mean, float deviation,
			      std::mt19937& rn)
{
  // [0.f, 2.f)で一様分布の乱数
  std::uniform_real_distribution<float> uniform(0.f, 2.f);

  // 平均mean, 標準偏差deviationでの正規分布
  std::normal_distribution<float> normal(mean, deviation);

  particles.reserve(particles.size() + count);
  
  // 原点中心に直径方向に正規分布する粒子群を生成
  for (auto i = 0; i < count; ++i) {
    // 緯度方向
    const float cp(uniform(rn) - 1.f);
    const float sp(sqrt(1.f - cp * cp));

    // 経度方向
    const float t(glm::pi<float>() * uniform(rn));
    const float ct(cos(t)), st(sin(t));

    // 粒子群中心からの距離
    const float r(normal(rn));

    // 粒子を追加
    particles.emplace_back(center.x, center.y, center.z, r * sp * ct, r * sp * st, r * cp);
  }
}

void SceneBlob::update()
{
  // 前回リセットからの経過時間(秒)とフレーム毎経過時間(秒)
  float reset_delta = nekolib::clock::Clock::calc_delta_seconds(cur_, start_);
  float delta = nekolib::clock::Clock::calc_delta_seconds(cur_, prev_);
  
  if (reset_delta > reset_interval_) {
    blob_->init(init_data_);
    cur_ = nekolib::clock::Clock::create(0.f);
    prev_ = cur_.snapshot();
    start_ = cur_.snapshot();
  } else {
    prev_ = cur_.snapshot();
  }

  // 粒子位置/速度の更新
  comp_prog_.use();
  comp_prog_.set_uniform("dt", delta);
  blob_->update();

  check_gl_error(__FILE__, __LINE__);
  
  using namespace nekolib::input;
  Mouse m = nekolib::input::Manager::instance().mouse();
  Keyboard kb = nekolib::input::Manager::instance().keyboard();

  // imgui表示中はカメラ操作はしない
  // 'D'キー押し下げで切り替え
  if (imgui_) {
    if (kb.triggered(SDLK_d)) {
      imgui_ = false;
    }
    return;
  } else {
    if (kb.triggered(SDLK_d)) {
      imgui_ = true;
      return;
    }
  }
  
  static bool dragging = false;
  if (!dragging) {
    if (m.pushed(Mouse::Button::LEFT)) {
      dragging = true;
      drag_start_x_ = m.x();
      drag_start_y_ = m.y();
    }
  } else {
    update_rotation(m.x(), m.y());
    if (!m.pushed(Mouse::Button::LEFT)) {
      dragging = false;
      orig_ = rot_;
    }
  }
}

void SceneBlob::render()
{
  static vec3 bg(0.1, 0.2, 0.3);
  static vec3 point_color(1.f, 1.f, 1.f);
  
  
  if (imgui_) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Once);
    ImGui::Begin("color config", &imgui_, IMGUI_SIMPLE_DIALOG_FLAGS);

    ImGui::ColorEdit3("background", &bg.x);
    ImGui::ColorEdit3("point color", &point_color.x);
    ImGui::End();
  }

  glClearColor(bg.x, bg.y, bg.z, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  prog_.use();
  prog_.set_uniform("color", point_color);
  
  model_ = glm::mat4(rot_);
  
  prog_.set_uniform("MVP", proj_ * view_ * model_);
  blob_->render();

  check_gl_error(__FILE__, __LINE__);
}

bool SceneBlob::compile_and_link_shaders()
{
  if (!prog_.compile_shader_from_file("shader/blob.vs", ShaderType::VERTEX)) {
    fprintf(stderr, "Compiling vertex shader failed.\n%s\n", prog_.log().c_str());
    return false;
  }
  if (!prog_.compile_shader_from_file("shader/blob.fs", ShaderType::FRAGMENT)) {
    fprintf(stderr, "Compiling fragment shader failed.\n%s\n", prog_.log().c_str());
    return false;
  }

  if (!prog_.link()) {
    fprintf(stderr, "Linking shader program failed.\n%s\n", prog_.log().c_str());
    return false;
  }
  if (!prog_.valid()) {
    fprintf(stderr, "Validating program failed.\n%s\n", prog_.log().c_str());
    return false;
  }

  if (!comp_prog_.compile_shader_from_file("shader/blob.cs", ShaderType::COMPUTE)) {
    fprintf(stderr, "Compiling compute shader failed.\n%s\n", comp_prog_.log().c_str());
    return false;
  }
  if (!comp_prog_.link()) {
    fprintf(stderr, "Linking shader program failed.\n%s\n", comp_prog_.log().c_str());
    return false;
  }
  if (!comp_prog_.valid()) {
    fprintf(stderr, "Validating program failed.\n%s\n", comp_prog_.log().c_str());
    return false;
  }

  prog_.use();
  prog_.print_active_attribs();
  prog_.print_active_uniforms();

  return true;
}
