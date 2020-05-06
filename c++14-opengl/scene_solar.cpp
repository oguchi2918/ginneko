#include <vector>
#include <random>
#include <cmath>
#include <cstdio>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"

#include "scene_solar.hpp"
#include "defines.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "utils.hpp"

using glm::vec2;
using glm::vec3;
using glm::vec4;

using namespace nekolib::renderer;

// 質点の状態(VBO用)
struct Point {
  alignas(4) float mass; // 質量
  alignas(16) vec3 position; // 位置
  alignas(16) vec3 velocity; // 速度
  alignas(16) vec3 position_temp; // 計算途中の値
  alignas(16) vec3 velocity_temp; // 計算途中の値

  Point(float m, vec2 p, vec2 v)
    : mass(m), position(p, 0.0), velocity(v, 0.0), position_temp(p, 0.0), velocity_temp(v, 0.0){}
};

// 物理パラメーター(UBO用)
struct PhysicParams {
  alignas(4) uint point_num; // 質点数
  alignas(4) float dt; // タイムステップ
  alignas(4) float g; // 重力加速度
  alignas(4) float r_threshold; // 引力発生距離閾値
};

// 計算方法
// Euler法(1次Runge-Kutta), Heunの公式(2次Runge-Kutta),
// 古典的Runge-Kutta法(4次Runge-Kutta), 速度Verlet法の4種類
enum class CompMethod { EULER = 0, HEUN = 1, RK = 2, VVER = 3 };

// 点描画のvertex buffer管理クラス
// 1個だけ作ってunique_ptrに放り込むのでコピー&ムーブ不可の方針で.
class PointsBuffer
{
public:
  PointsBuffer(const Points&, const PhysicParams*,
	       Program&, Program&, Program&, Program&, Program&);
  ~PointsBuffer() = default;

  PointsBuffer(const PointsBuffer&) = delete;

  PointsBuffer& operator=(const PointsBuffer&) = delete;
  PointsBuffer(PointsBuffer&&) = delete;
  PointsBuffer& operator=(PointsBuffer&&) = delete;

  void render() const;
  void update();
  void calc_momentum_and_energy(float*, float*, float*) const;

  void reset();
  void set_comp_method(CompMethod);
  int comp_method() { return static_cast<int>(comp_method_); }
private:
  vec3 calc_momentum(const Point*) const;
  float calc_T(const Point*) const;
  float calc_U(const Point*) const;

  void init_rk();
  void init_vver();

  static const int buffer_num_ = 3;
  gl::Vao vao_[buffer_num_];
  gl::VertexBuffer vbo_[buffer_num_];
  gl::UniformBuffer<PhysicParams> ubo_;

  size_t current_; // どのvbo_を表示対象とするか[0, bufer_num_)
  CompMethod comp_method_; // 計算方法

  // 初期状態
  const Points init_data_;
  const PhysicParams physic_params_;
  const double init_energy_;

  // 計算シェーダー
  // SSBO経由でvbo_(のGPU側にあるデータ)を書き換える
  Program& rk_init_prog_;
  Program& rk_prog_;
  Program& rk_finish_prog_;
  Program& vver_init_prog_;
  Program& vver_prog_;
};

PointsBuffer::PointsBuffer(const Points& points, const PhysicParams* physic_params,
			   Program& rk_init, Program& rk, Program& rk_finish,
			   Program& vver_init, Program& vver)
  : ubo_(physic_params), current_(0), comp_method_(CompMethod::VVER),
    init_data_(points), physic_params_(*physic_params),
    init_energy_(calc_U(&points[0]) + calc_T(&points[0])),
    rk_init_prog_(rk_init), rk_prog_(rk), rk_finish_prog_(rk_finish),
    vver_init_prog_(vver_init), vver_prog_(vver)
{
  assert(points.size() == physic_params_.point_num);

  for (size_t i = 0; i < buffer_num_; ++i) {
    vbo_[i].bind();
    glBufferData(GL_ARRAY_BUFFER, physic_params_.point_num * sizeof(Point), &points[0], GL_DYNAMIC_DRAW);


    vao_[i].bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), BUFFER_OFFSETOF(Point, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Point), BUFFER_OFFSETOF(Point, mass));
    vao_[i].bind(false);
  }

  // 物理パラメーターは全計算シェーダーで同じものを使用するのでUBOで設定
  ubo_.select(0);

  if (comp_method_ == CompMethod::VVER) {
    init_vver();
  } else {
    init_rk();
  }
}

// velocity Verlet法用にposition_temp, velocity_tempを初期化する
// 専用計算シェーダー呼び出し
void PointsBuffer::init_vver()
{
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_[current_].handle());
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_[(current_ + 1) % buffer_num_].handle());
  
  vver_init_prog_.use();
  vver_init_prog_.set_uniform_block("PhysicParams", 0);

  // 計算シェーダーを起動
  glDispatchCompute(physic_params_.point_num, 1, 1);
  
  // バッファ交代  
  current_ = (current_ + 1) % buffer_num_;
  
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  check_gl_error(__FILE__, __LINE__);
}

// Runge Kutta法用にposition_temp, velocity_tempを初期化する
// 専用計算シェーダー呼び出し
void PointsBuffer::init_rk()
{
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_[current_].handle());
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_[(current_ + 1) % buffer_num_].handle());
  
  rk_init_prog_.use();
  rk_init_prog_.set_uniform_block("PhysicParams", 0);

  // 計算シェーダーを起動
  glDispatchCompute(physic_params_.point_num, 1, 1);

  // バッファ交代は不要
  
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  check_gl_error(__FILE__, __LINE__);
}

vec3 PointsBuffer::calc_momentum(const Point* p) const
{
  vec3 ans(0.0);
  for (size_t i = 0; i < physic_params_.point_num; ++i) {
    ans += p[i].mass * p[i].velocity;
  }

  return ans;
}

// 位置エネルギー総和を計算
float PointsBuffer::calc_U(const Point* p) const
{
  float ans(0.0);
  for (size_t i = 0; i < physic_params_.point_num; ++i) {
    for (size_t j = i + 1; j < physic_params_.point_num; ++j) {
      float r = glm::length(p[j].position - p[i].position);
      // 位置エネルギーは距離=閾値の時の値を最大値とする
      r = std::max(r, physic_params_.r_threshold);
      ans += physic_params_.g * p[i].mass * p[j].mass * std::log(r);
    }
  }

  return ans;
}

// 運動エネルギー総和を計算
float PointsBuffer::calc_T(const Point* p) const
{
  float ans(0.0);
  for (size_t i = 0; i < physic_params_.point_num; ++i) {
    ans += 0.5 * p[i].mass * glm::dot(p[i].velocity, p[i].velocity);
  }

  return ans;
}

void PointsBuffer::calc_momentum_and_energy(float* mx, float* my, float* e) const
{
  vbo_[current_].bind();
  auto pmapped = static_cast<const Point*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));

  auto M = calc_momentum(pmapped);
  auto U = calc_U(pmapped); auto T = calc_T(pmapped);
  *mx = M.x; *my = M.y; *e = U + T - init_energy_;

  glUnmapBuffer(GL_ARRAY_BUFFER);
}

// 計算方法変更
void PointsBuffer::set_comp_method(CompMethod cm)
{
  // Euler, Heunも内部的にはRunge-Kuttaのシェーダーを使用している
  // velocity verletとRunge-Kuttaの切り替え時に
  // 不整合が起きないように初期化シェーダーを実行する必要有り
  
  if (comp_method_ == CompMethod::VVER && cm != comp_method_) {
    // velocity verlet -> Runge-Kutta
    comp_method_ = cm;
    init_rk();
  } else if (cm != comp_method_) {
    comp_method_ = cm;
    if (cm == CompMethod::VVER) {
      // Runge-Kutta -> velocity verlet
      init_vver();
    }
  }
}

void PointsBuffer::render() const
{
  vao_[current_].bind();
  glDrawArrays(GL_POINTS, 0, physic_params_.point_num);
}

// 位置/速度更新用計算シェーダー起動
void PointsBuffer::update()
{
  switch (comp_method_) {
  case CompMethod::EULER: // Euler法
    // 実装は1次のRunge-Kutta法
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_[current_].handle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_[(current_ + 1) % buffer_num_].handle());
  
    rk_finish_prog_.use();
    rk_finish_prog_.set_uniform_block("PhysicParams", 0);
    rk_finish_prog_.set_uniform("x_dt2", 1.f);

    // 計算シェーダーを起動
    glDispatchCompute(physic_params_.point_num, 1, 1);

    check_gl_error(__FILE__, __LINE__);

    // バッファ交代  
    current_ = (current_ + 1) % buffer_num_;
    break;
  case CompMethod::HEUN: // Heunの方法(改良オイラー法)
    // 実装は2次のRunge-Kutta法
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_[current_].handle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_[current_].handle());    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vbo_[(current_ + 1) % buffer_num_].handle());

    rk_prog_.use();
    rk_prog_.set_uniform_block("PhysicParams", 0);
    rk_prog_.set_uniform("x_dt1", 1.0f);
    rk_prog_.set_uniform("x_dt2", 0.5f);

    glDispatchCompute(physic_params_.point_num, 1, 1);    
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_[(current_ + 1) % buffer_num_].handle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_[current_].handle());

    rk_finish_prog_.use();
    rk_finish_prog_.set_uniform_block("PhysicParams", 0);
    rk_finish_prog_.set_uniform("x_dt2", 0.5f);
    
    glDispatchCompute(physic_params_.point_num, 1, 1);    

    break;
  case CompMethod::RK: // 古典的四次Runge-Kutta法
    // 4回も力の計算をして同期待ちするので重い
    // 宅環境だとこいつだけエネルギー計算までするとフレーム落ちする
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_[current_].handle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_[current_].handle());    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vbo_[(current_ + 1) % buffer_num_].handle());

    rk_prog_.use();
    rk_prog_.set_uniform_block("PhysicParams", 0);
    rk_prog_.set_uniform("x_dt1", 0.5f);
    rk_prog_.set_uniform("x_dt2", 1.f / 6.f);

    glDispatchCompute(physic_params_.point_num, 1, 1);    
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_[current_].handle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_[(current_ + 1) % buffer_num_].handle());    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vbo_[(current_ + 2) % buffer_num_].handle());

    rk_prog_.set_uniform_block("PhysicParams", 0);
    rk_prog_.set_uniform("x_dt1", 0.5f);
    rk_prog_.set_uniform("x_dt2", 1.f / 3.f);

    glDispatchCompute(physic_params_.point_num, 1, 1);    
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_[current_].handle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_[(current_ + 2) % buffer_num_].handle());    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vbo_[(current_ + 1) % buffer_num_].handle());

    rk_prog_.set_uniform_block("PhysicParams", 0);
    rk_prog_.set_uniform("x_dt1", 1.f);
    rk_prog_.set_uniform("x_dt2", 1.f / 3.f);

    glDispatchCompute(physic_params_.point_num, 1, 1);    
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_[(current_ + 1) % buffer_num_].handle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_[current_].handle());

    rk_finish_prog_.use();
    rk_finish_prog_.set_uniform_block("PhysicParams", 0);
    rk_finish_prog_.set_uniform("x_dt2", 1.f / 6.f);
    
    glDispatchCompute(physic_params_.point_num, 1, 1);    
    break;
  case CompMethod::VVER: // velocity verlet
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_[current_].handle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_[(current_ + 1) % buffer_num_].handle());
  
    vver_prog_.use();
    vver_prog_.set_uniform_block("PhysicParams", 0);

    // 計算シェーダーを起動
    glDispatchCompute(physic_params_.point_num, 1, 1);

    check_gl_error(__FILE__, __LINE__);

    // バッファ交代  
    current_ = (current_ + 1) % buffer_num_;
    break;
  default:
    assert(!"This must not be happen!");
    break;
  }
}

// 質点を初期状態に戻す
void PointsBuffer::reset()
{
  for (size_t i = 0; i < 2; ++i) {
    vbo_[i].bind();
    glBufferData(GL_ARRAY_BUFFER, physic_params_.point_num * sizeof(Point), &init_data_[0], GL_DYNAMIC_DRAW);

    vao_[i].bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), BUFFER_OFFSETOF(Point, position));
    vao_[i].bind(false);
  }

  current_ = 0;
  
  if (comp_method_ == CompMethod::VVER) {
    init_vver();
  } else {
    init_rk();
  }

  check_gl_error(__FILE__, __LINE__);
}

SceneSolar::SceneSolar() : current_(0), locus_(false), pause_(false), cme_(true), imgui_(true) {}
SceneSolar::~SceneSolar(){}

// 初期データの速度補正
// 系全体の重心が画面中心から動かないようにしたいので
// 系全体の運動量->速度を計算して各質点の速度を相対速度に変更
// この関数は一度だけ呼ばれる
void SceneSolar::correct_init_data(Points& init_data)
{
  static bool called = false;
  assert(called == false);
  
  vec3 mom_total(0.0);
  float mass_total(0.0);

  // 系全体の運動量を計算
  for (const auto& point : init_data) {
    mom_total += point.mass * point.velocity;
    mass_total += point.mass;
  }

  // 各質点の速度を補正
  vec3 system_vel = mom_total / mass_total;
  for (auto& point : init_data) {
    point.velocity -= system_vel;
    point.velocity_temp -= system_vel;
  }

  called = true;
}

bool SceneSolar::setup_fbo()
{
  for (size_t i = 0; i < 2; ++i) {
    fbo_[i].bind();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_tex_[i].handle(), 0);

    if (check_fbo_status(__FILE__, __LINE__)) {
      return false;
    }

    GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, };
    glDrawBuffers(1, draw_buffers);

    // 画面クリア
    glClearColor(0.1f, 0.1f, 0.1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
  
    fbo_[i].bind(false);
  }
  
  return true;
}

// point_num(-1)個の原点から距離max_r内に一様分布する点群をランダム生成
// 速さは接線方向に絶対値0.3以上max_v以下の一様分布
// 質量はmax_m以下の一様分布する整数値
//
// 最初の点データだけは固定だったり関数名と異なり色々恣意的なのは仕様
Points SceneSolar::generate_random_init_data(size_t point_num,
					     unsigned max_m, float max_r, float max_v)
{
  // 表示の都合上これ位の範囲が実用的かなあ
  // 速すぎるとアニメーションが飛び飛びになるし
  assert(point_num > 0 && point_num <= 1000);
  assert(max_m <= 5);
  assert(max_r > 0 && max_r <= 1.f);
  assert(max_v > 0.3 && max_v <= 3.f);
  
  Points ans;

  ans.reserve(point_num);
  
  // 最初の点データは固定値
  ans.emplace_back(5000.0, vec2(0.0, 0.0), vec2(0.0, 0.0));

  std::random_device rd;
  std::mt19937 mt(rd());

  std::uniform_int_distribution<int> ui(1, max_m);
  std::uniform_real_distribution<float> ur(0.f, 0.5 * max_r * max_r);
  std::uniform_real_distribution<float> ut(0.f, glm::two_pi<float>());

  // 残りpoint_num - 1個を生成
  for (size_t i = 0; i < point_num - 1; ++i) {
    // 質量
    // 乱数が整数値なのは巨大原子とか電子雲のイメージがあっただけ
    // (特に深い意味はない)
    float mass = static_cast<float>(ui(mt));
    
    // 位置
    // 極座標(r, t)を使用
    float r = std::sqrt(2 * ur(mt));
    float t = ut(mt);
    vec2 pos(r * std::cos(t), r * std::sin(t));

    // 速度
    // 半径方向に初速を与えても単振動するだけでつまらないので
    // 接線方向にランダムな大きさ(最低値は0.3)の初速を与える
    std::uniform_real_distribution<float> uv(0.3, max_v);
    r = uv(mt);
    vec3 temp = r * glm::normalize(glm::cross(vec3(pos, 0.f), vec3(0.f, 0.f, 1.f)));
    vec2 vel(temp.x, temp.y);

    ans.emplace_back(mass, pos, vel);
  }

  return ans;
}

bool SceneSolar::init()
{
  if (!compile_and_link_shaders()) {
    return false;
  }

  cur_ = nekolib::clock::Clock::create(0.f);
  prev_ = cur_.snapshot();

  quad_ = Quad::create(0.f, 0.f, 1.f, 1.f);

  for (size_t i = 0; i < 2; ++i) {
    render_tex_[i] = Texture::create(ScreenManager::width(), ScreenManager::height(), TextureFormat::RGBA8);
    render_tex_[i].bind(1 + i);
  }

  if (!setup_fbo()) {
    return false;
  }

  // 初期位置/速度
  // Points init_data = { Point(5000.0, vec2(0.0, 0.0), vec2(0.0, 0.0)),
  // 		       Point(1.0, vec2(0.0, -0.2), vec2(1.3, 0.4)),
  // 		       Point(1.0, vec2(0.0, -0.24), vec2(1.2, 0.4)),
  // 		       Point(1.0, vec2(0.0, -0.28), vec2(1.1, 0.4)),
  // 		       Point(1.0, vec2(0.0, -0.32), vec2(1.0, 0.4)),
  // 		       Point(1.0, vec2(0.0, -0.36), vec2(0.9, 0.4)),
  // 		       Point(1.0, vec2(0.0, -0.4), vec2(0.8, 0.4)),
  // 		       Point(1.0, vec2(0.0, -0.44), vec2(0.7, 0.4)),
  // 		       Point(1.0, vec2(0.0, -0.48), vec2(0.7, 0.4)),
  // 		       Point(1.0, vec2(0.0, -0.52), vec2(0.7, 0.4)),
  // 		       Point(1.0, vec2(0.0, -0.56), vec2(0.7, 0.4)),
  // 		       Point(1.0, vec2(0.0, -0.6), vec2(0.7, 0.4)),
  // 		       Point(1.0, vec2(0.0, -0.64), vec2(0.7, 0.4)),
  // 		       Point(1.0, vec2(0.0, -0.68), vec2(0.7, 0.4)) };

  // これ位の初速だと粒子が画面外に飛び出ない模様
  // (まあ飛び出ても理論上いつか確実に戻ってくるんだけど)
  // 重力井戸に未来永劫囚われ続ける星々の姿をお楽しみください
  Points init_data = generate_random_init_data(500, 5.f, 0.8f, 1.2f);

  // 速度補正
  correct_init_data(init_data);

  // 物理パラメーター
  const PhysicParams physic_param =
    { static_cast<uint>(init_data.size()),
      1.0 / 300,
      0.0002,
      0.05,
    };
  
  points_buffer_ = std::make_unique<PointsBuffer>(init_data, &physic_param,
						  rk_init_prog_, rk_prog_, rk_finish_prog_,
						  vver_init_prog_, vver_prog_);

  glEnable(GL_PROGRAM_POINT_SIZE);

  fprintf(stdout, "Press 1/2/3/4 key to change calculate method.\n");
  fprintf(stdout, "1: Euler method, 2: Heun method, 3: Runge-Kutta, 4: Velocity verlet\n");
  fprintf(stdout, "Press 'l' key to show/erase locus.\n");
  fprintf(stdout, "And...\n");
  fprintf(stdout, "Press 'd' key to show/erase dialog.\n");

  return true;
}

void SceneSolar::update()
{
  // float dt = nekolib::clock::Clock::calc_delta_seconds(cur_, prev_);
  // prev_ = cur_.snapshot();
  // fprintf(stderr, "%f\n", dt);

  using namespace nekolib::input;
  Keyboard kb = nekolib::input::Manager::instance().keyboard();

  // 'D'キー押し下げでダイアログ表示
  if (imgui_) {
    if (kb.triggered(SDLK_d)) {
      imgui_ = false;
    }
  } else {
    if (kb.triggered(SDLK_d)) {
      imgui_ = true;
    }
  }

  if (kb.triggered(SDLK_1)) {
    points_buffer_->set_comp_method(CompMethod::EULER);
  } else if (kb.triggered(SDLK_2)) {
    points_buffer_->set_comp_method(CompMethod::HEUN);
  } else if (kb.triggered(SDLK_3)) {
    points_buffer_->set_comp_method(CompMethod::RK);
  } else if (kb.triggered(SDLK_4)) {
    points_buffer_->set_comp_method(CompMethod::VVER);
  }

  if (kb.triggered(SDLK_l)) {
    locus_ = !locus_;
  }

  // 位置と速度を更新
  if (!pause_) {
    points_buffer_->update();
  }
}

void SceneSolar::render()
{
  // pass 1(裏画面)
  fbo_[current_].bind();

  glClear(GL_COLOR_BUFFER_BIT);

  if (locus_) {
    // 前フレームの描画結果の透明度を上げてアルファブレンド
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    quad_prog_.use();
    quad_prog_.set_uniform("Tex", 1 + (1 - current_)); // 前フレームの描画結果
    quad_prog_.set_uniform("Factor", 0.005f);
    quad_.render();
  }

  // points_buffer->update()の並列計算を同期待ち
  // 理屈上MemoryBarrier()はキリギリまで遅らせた方が余計なWaitが入らない筈
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  if (imgui_) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Once);
    ImGui::Begin("config", &imgui_, IMGUI_SIMPLE_DIALOG_FLAGS);

    ImGui::Checkbox("Calc momentum & energy", &cme_);
    if (cme_) {
      float mx, my, en;
      points_buffer_->calc_momentum_and_energy(&mx, &my, &en);
      ImGui::Text("memontum: %+f, %+f, energy: %+f", mx, my, en);
    } else {
      ImGui::NewLine();
    }
    
    int cm = points_buffer_->comp_method();
    ImGui::RadioButton("Euler", &cm, 0); ImGui::SameLine();
    ImGui::RadioButton("Heun", &cm, 1); ImGui::SameLine();
    ImGui::RadioButton("Runge Kutta", &cm, 2); ImGui::SameLine();
    ImGui::RadioButton("Velocity Verlet", &cm, 3);

    ImGui::Checkbox("show locus", &locus_);
    ImGui::SameLine(0, 150);
    
    if (pause_) {
      if (ImGui::Button("Start")) {
	pause_ = false;
      }
    } else {
      if (ImGui::Button("Pause")) {
	pause_ = true;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
      // locus_ = false;
      points_buffer_->reset();
      glClear(GL_COLOR_BUFFER_BIT);      
    }
    ImGui::End();
    
    switch (cm) {
    case 0:
      points_buffer_->set_comp_method(CompMethod::EULER);
      break;
    case 1:
      points_buffer_->set_comp_method(CompMethod::HEUN);
      break;
    case 2:
      points_buffer_->set_comp_method(CompMethod::RK);
      break;
    case 3:
      points_buffer_->set_comp_method(CompMethod::VVER);
      break;
    default:
      assert(!"This must not be happen!");
      break;
    }
  }

  // 点描画
  points_prog_.use();
  points_buffer_->render();

  check_gl_error(__FILE__, __LINE__);

  fbo_[current_].bind(false);
  current_ = 1 - current_; // 裏画面交代

  // pass 2 (裏画面を表画面に描画)
  glDisable(GL_BLEND);
  quad_prog_.use();
  quad_prog_.set_uniform("Tex", 1);
  quad_prog_.set_uniform("Factor", 0.f);
  quad_.render();
}

bool SceneSolar::compile_and_link_shaders()
{
  using Names = std::vector<std::string>;

  // 描画用
  if (!points_prog_.build_program_from_files(Names{ "shader/solar.vs", "shader/solar.fs" })) {
    return false;
  }
  if (!quad_prog_.build_program_from_files(Names{ "shader/quad.vs", "shader/solar_quad.fs" })) {
    return false;
  }

  // 計算用
  if (!rk_init_prog_.build_program_from_files(Names{ "shader/solar_rk_init.cs" })) {
    return false;
  }
  if (!rk_prog_.build_program_from_files(Names{ "shader/solar_rk.cs" })) {
    return false;
  }
  if (!rk_finish_prog_.build_program_from_files(Names{ "shader/solar_rk_end.cs" })) {
    return false;
  }
  
  if (!vver_init_prog_.build_program_from_files(Names{ "shader/solar_vver_init.cs" })) {
    return false;
  }
  if (!vver_prog_.build_program_from_files(Names{ "shader/solar_vver.cs" })) {
    return false;
  }

  points_prog_.use();
  points_prog_.print_active_attribs();
  points_prog_.print_active_uniforms();

  return true;
}
