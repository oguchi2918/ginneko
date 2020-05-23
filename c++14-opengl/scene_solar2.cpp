#include <vector>
#include <random>
#include <cmath>
#include <cstdio>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "imgui/imgui.h"

#include "scene_solar2.hpp"
#include "defines.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "utils.hpp"

using glm::vec3;
using glm::vec4;

using glm::mat3;
using glm::mat4;

using namespace nekolib::renderer;

// 質点の状態(VBO用)
struct Point {
  alignas(4) float mass; // 質量
  alignas(16) vec3 position; // 位置
  alignas(16) vec3 velocity; // 速度
  alignas(16) vec3 position_temp; // 計算途中の値
  alignas(16) vec3 velocity_temp; // 計算途中の値

  Point(float m, vec3 p, vec3 v)
    : mass(m), position(p), velocity(v), position_temp(p), velocity_temp(v){}
};

// 物理パラメーター(UBO用)
struct PhysicParams {
  alignas(4) uint point_num; // 質点数
  alignas(4) float dt; // タイムステップ
  alignas(4) float g; // 重力加速度
  alignas(4) float r_threshold; // 引力発生距離閾値
};

// 点描画のvertex buffer管理クラス
// 1個だけ作ってunique_ptrに放り込むのでコピー&ムーブ不可の方針で.
class PointsBuffer
{
public:
  PointsBuffer(const Points&, const PhysicParams*,
	       Program&, Program&);
  ~PointsBuffer() = default;

  PointsBuffer(const PointsBuffer&) = delete;

  PointsBuffer& operator=(const PointsBuffer&) = delete;
  PointsBuffer(PointsBuffer&&) = delete;
  PointsBuffer& operator=(PointsBuffer&&) = delete;

  void render_points() const;
  void update();
  void get_info(vec3*, vec3*, vec3*, float*) const;

  void reset();
private:
  vec3 calc_momentum(const Point*) const noexcept;
  float calc_T(const Point*) const noexcept;
  float calc_U(const Point*) const noexcept;

  void init_vver();

  // 質点群の配列を格納するvertex buffer object他
  // 計算シェーダーでの更新用に2個必要
  static const int buffer_num_ = 2;
  gl::Vao vao_[buffer_num_];
  gl::VertexBuffer vbo_[buffer_num_];
  gl::UniformBuffer<PhysicParams> ubo_;

  size_t current_; // どのvbo_を表示対象とするか[0, bufer_num_)

  // 初期状態
  const Points init_data_;
  const PhysicParams physic_params_;
  const float init_energy_;
  const vec3 init_momentum_;

  // 計算シェーダー
  // SSBO経由でvbo_(のGPU側にあるデータ)を書き換える
  Program& vver_init_prog_;
  Program& vver_prog_;
};

PointsBuffer::PointsBuffer(const Points& points, const PhysicParams* physic_params,
			   Program& vver_init, Program& vver)
  : ubo_(physic_params), current_(0),
    init_data_(points), physic_params_(*physic_params),
    init_energy_(calc_U(&points[0]) + calc_T(&points[0])),
    init_momentum_(calc_momentum(&points[0])),
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

  init_vver();
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

vec3 PointsBuffer::calc_momentum(const Point* p) const noexcept
{
  vec3 ans(0.0);
  for (size_t i = 0; i < physic_params_.point_num; ++i) {
    ans += p[i].mass * p[i].velocity;
  }

  return ans;
}

// 位置エネルギー総和を計算
float PointsBuffer::calc_U(const Point* p) const noexcept
{
  float ans(0.0);
  for (size_t i = 0; i < physic_params_.point_num; ++i) {
    for (size_t j = i + 1; j < physic_params_.point_num; ++j) {
      float r = glm::length(p[j].position - p[i].position);
      // 位置エネルギーは距離=閾値の時の値を最小値とする
      r = std::max(r, physic_params_.r_threshold);
      ans += physic_params_.g * p[i].mass * p[j].mass / r;
    }
  }

  return ans;
}

// 運動エネルギー総和を計算
float PointsBuffer::calc_T(const Point* p) const noexcept
{
  float ans(0.0);
  for (size_t i = 0; i < physic_params_.point_num; ++i) {
    ans += 0.5 * p[i].mass * glm::dot(p[i].velocity, p[i].velocity);
  }

  return ans;
}

// 太陽の位置と系全体の運動量及びエネルギーを取得する
void PointsBuffer::get_info(vec3* sun_pos, vec3* sun_vel, vec3* m, float* e) const
{
  vbo_[current_].bind();
  auto pmapped = static_cast<const Point*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));

  auto M = calc_momentum(pmapped);
  auto U = calc_U(pmapped); auto T = calc_T(pmapped);
  *m = M - init_momentum_; *e = U + T - init_energy_;
  *sun_pos = pmapped[0].position; *sun_vel = pmapped[0].velocity;

  glUnmapBuffer(GL_ARRAY_BUFFER);
}

void PointsBuffer::render_points() const
{
  vao_[current_].bind();
  glDrawArrays(GL_POINTS, 0, physic_params_.point_num);
  vao_[current_].bind(false);
}

// 位置/速度更新用計算シェーダー起動
void PointsBuffer::update()
{
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo_[current_].handle());
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo_[(current_ + 1) % buffer_num_].handle());
  
  vver_prog_.use();
  vver_prog_.set_uniform_block("PhysicParams", 0);

  // 計算シェーダーを起動
  glDispatchCompute(physic_params_.point_num, 1, 1);

  check_gl_error(__FILE__, __LINE__);

  // バッファ交代  
  current_ = (current_ + 1) % buffer_num_;
}

// 質点を初期状態に戻す
void PointsBuffer::reset()
{
  for (size_t i = 0; i < buffer_num_; ++i) {
    vbo_[i].bind();
    glBufferData(GL_ARRAY_BUFFER, physic_params_.point_num * sizeof(Point), &init_data_[0], GL_DYNAMIC_DRAW);

  }

  current_ = 0;
  
  init_vver();

  check_gl_error(__FILE__, __LINE__);
}

// ストーカー御用達カメラ
TrackQuatCamera::TrackQuatCamera(vec3 pos, vec3 target, vec3 up) noexcept
  : target_(target), distance_(glm::length(target - pos)),
    org_front_(glm::normalize(target_ - pos)), org_up_(calc_up(org_front_, up)),
    front_(org_front_), up_(org_up_)
{
  cq_ = glm::quat(1.f, 0.f, 0.f, 0.f);
}

vec3 TrackQuatCamera::calc_up(vec3 front, vec3 up) noexcept
{
  vec3 right = glm::normalize(glm::cross(front, up));
  return glm::normalize(glm::cross(right, front));
}
	       
void TrackQuatCamera::reset_delta() noexcept
{
  cq_ = cq_ * dq_;
  dq_ = glm::quat(1.f, 0.f, 0.f, 0.f);
}

void TrackQuatCamera::update_dirs() noexcept
{
  front_ = glm::normalize(cq_ * dq_ * org_front_);
  up_ = glm::normalize(cq_ * dq_ * org_up_);
}

void TrackQuatCamera::update() noexcept
{
  using namespace nekolib::input;
  Mouse m = nekolib::input::Manager::instance().mouse();

  int x = m.x(), y = m.y();
  if (drag_) {
    if (m.pushed(Mouse::Button::LEFT)) { // drag継続
      process_drag(x - drag_sx_, y - drag_sy_);
    } else { // drag終了
      process_drag(x - drag_sx_, y - drag_sy_);
      reset_delta();
      drag_ = false;
    }
  } else if (m.pushed(Mouse::Button::LEFT)) { // drag開始
    drag_ = true;
    drag_sx_ = x; drag_sy_ = y;
  }
}

void TrackQuatCamera::process_drag(int dx, int dy) noexcept
{
  if (abs(dx) <= 0.01 && abs(dy) <= 0.01) {
    return;
  }

  // rotate camera around target
  float fx = static_cast<float>(-dx) / ScreenManager::width();
  float fy = static_cast<float>(-dy) / ScreenManager::height();

  float ar = sqrt(fx * fx + fy * fy) * glm::two_pi<float>();
  dq_ = glm::rotate(glm::quat(1.f, 0.f, 0.f, 0.f), ar, glm::vec3(fy, fx, 0.f));
  update_dirs();
}

// 追跡対象を再設定
// カメラ自身の姿勢は変更しないで並行移動する
void TrackQuatCamera::set_target(glm::vec3 target) noexcept
{
  target_ = target;
}

mat4 TrackQuatCamera::view_matrix() const noexcept
{
  return glm::lookAt(target_ - (distance_ * front_), target_, up_);
}

SceneSolar2::SceneSolar2(unsigned n) : camera_(glm::vec3(0.f, 0.f, 3.f),
					       glm::vec3(0.f, 0.f, 0.f)),
				       current_(0), point_num_(n),
				       locus_(false), pause_(false), imgui_(true) {}
SceneSolar2::~SceneSolar2(){}

// 軸表示
// 3D宇宙空間は基本的に天地無用だが、回転操作がやり難いので参考までに
//
// 太陽の進行方向をz軸として赤線で表示
// 残り2軸は適当に前フレームの値から算出している
void SceneSolar2::render_axis(vec3 sun_pos, vec3 sun_vel)
{
  static vec3 up(0.f, 1.f, 0.f);
  static vec3 front;

  // 太陽の速度が0だった場合は前フレームの値を流用する
  if (glm::cross(sun_vel, up) != vec3(0.f, 0.f, 0.f)) {
    front = glm::normalize(sun_vel);
  }
  vec3 right = glm::normalize(glm::cross(up, front));
  up = glm::normalize(glm::cross(front, right));

  model_ = glm::translate(mat4(1.f), sun_pos);
  model_ *= mat4(mat3(right, up, front));
  model_ *= glm::scale(mat4(1.f), vec3(1.5f, 1.5f, 1.5f));
  
  axis_prog_.use();
  axis_prog_.set_uniform("MVP", proj_ * camera_.view_matrix() * model_);
  
  axis_vao_.bind();
  glDrawArrays(GL_LINES, 0, 6);
  axis_vao_.bind(false);
}

bool SceneSolar2::setup_fbo()
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
    glClearColor(0.1f, 0.1f, 0.2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
  
    fbo_[i].bind(false);
  }
  
  return true;
}

// point_num(-1)個の原点から距離max_r内のxy平面上に一様分布する点群をランダム生成
// 速さは接線方向に絶対値0.3以上max_v以下の一様分布
// 質量はmax_m以下の一様分布する整数値
//
// 本当は3次元にバラマキたかったがあまり綺麗な画にならなかった
// いびつな螺旋より渦巻の方が個人的には好み
// 最初の点データだけは固定だったり色々恣意的なのは仕様
Points SceneSolar2::generate_init_data(size_t point_num,
				       unsigned max_m, float max_r, float max_v)
{
  // 表示の都合上これ位の範囲が実用的かなあ
  // 速すぎるとアニメーションが飛び飛びになるし
  assert(point_num > 0 && point_num <= 2000);
  assert(max_m <= 5);
  assert(max_r > 0 && max_r <= 1.0);
  assert(max_v > 0.3 && max_v <= 3.0);
  
  Points ans;

  ans.reserve(point_num);

  // 太陽の初速(表示上の都合で0にしたくない)
  const vec3 sun_vel(0.f, 0.f, 1.f);
  // 最初の点データは固定値
  ans.emplace_back(3000.0, vec3(0.f, 0.f, 0.f), sun_vel);

  std::random_device rd;
  std::mt19937 mt(rd());

  std::uniform_int_distribution<int> ui(1, max_m);
  std::uniform_real_distribution<float> ur(0.0, 0.5 * max_r * max_r);
  std::uniform_real_distribution<float> ut(0.0, glm::two_pi<float>());
  std::uniform_real_distribution<float> up(-1.f, 1.f);

  // 残りpoint_num - 1個をz=0の平面上に生成
  // 本当は3Dらしく球状にバラマキたかったが…
  for (size_t i = 0; i < point_num - 1; ++i) {
    // 質量
    // 乱数が整数値なのは巨大原子とか電子雲のイメージがあっただけ
    // (特に深い意味はない)
    float mass = static_cast<float>(ui(mt));
    
    // 位置
    // 極座標(r, t)を使用
    float r = std::sqrt(2 * ur(mt));
    float t = ut(mt);
    vec3 pos(r * std::cos(t), r * std::sin(t), 0.f);

    // 速度
    // 半径方向に初速を与えても単振動するだけでつまらないので
    // 接線方向にランダムな大きさ(最低値は0.3)の初速を与える
    std::uniform_real_distribution<float> uv(0.3, max_v);
    r = uv(mt);
    vec3 vel = r * glm::normalize(glm::cross(pos, vec3(0.f, 0.f, 1.f)));

    // 初速には太陽の初速分を加算しておく
    ans.emplace_back(mass, pos, sun_vel + vel);
  }

  return ans;
}

bool SceneSolar2::init()
{
  if (!compile_and_link_shaders()) {
    return false;
  }

  // 軸表示用データ
  std::vector<vec3> axis_data =
    { vec3(0.f, 0.f, 0.f), vec3(1.f, 0.f, 0.f),
      vec3(0.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f),
      vec3(0.f, 0.f, 0.f), vec3(0.f, 0.f, 1.f), };
      
  axis_vbo_.bind();
  glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vec3), &axis_data[0], GL_STATIC_DRAW);
  
  axis_vao_.bind();
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), BUFFER_OFFSET(0));
  axis_vao_.bind(false);
  
  cur_ = nekolib::clock::Clock::create(0.f);
  prev_ = cur_.snapshot();

  quad_ = Quad::create(0.f, 0.f, 1.f, 1.f);

  for (size_t i = 0; i < 2; ++i) {
    // 裏画面用テクスチャ
    // 軌跡表示時にアルファブレンドするのでRGBA
    render_tex_[i] = Texture::create(ScreenManager::width(), ScreenManager::height(), TextureFormat::RGBA8);
    render_tex_[i].bind(1 + i);
  }

  proj_ = glm::perspective(glm::radians(60.f), ScreenManager::aspectf(), 0.1f, 10.f);

  if (!setup_fbo()) {
    return false;
  }

  // 初期位置/速度
  // Points init_data = { Point(5000.0, vec3(0.f, 0.f, 0.f), vec3(0.f, 0.f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.2f, 0.f), vec3(1.3f, 0.4f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.24f, 0.f), vec3(1.2f, 0.4f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.28f, 0.f), vec3(1.1f, 0.4f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.32f, 0.f), vec3(1.f, 0.4f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.36f, 0.f), vec3(0.9f, 0.4f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.4f, 0.f), vec3(0.8f, 0.4f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.44f, 0.f), vec3(0.7f, 0.4f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.48f, 0.f), vec3(0.7f, 0.4f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.52f, 0.f), vec3(0.7f, 0.4f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.56f, 0.f), vec3(0.7f, 0.4f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.6f, 0.f), vec3(0.7f, 0.4f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.64f, 0.f), vec3(0.7f, 0.4f, 1.f)),
  // 		       Point(1.f, vec3(0.f, -0.68f, 0.f), vec3(0.7f, 0.4f, 1.f)) };

  // 割と適当
  Points init_data = generate_init_data(point_num_, 5.0, 1.0, 1.2);

  // 物理パラメーター
  // 計算シェーダーを逆二乗則にするのでdtは小さめ
  // これでもまだ近日点で速度が上がり過ぎてアニメーションが飛び点になる
  const PhysicParams physic_param =
    { static_cast<uint>(init_data.size()),
      1.0 / 800,
      0.0002,
      0.01,
    };
  
  points_buffer_ = std::make_unique<PointsBuffer>(init_data, &physic_param,
						  vver_init_prog_, vver_prog_);


  fprintf(stdout, "Press 'l' key to show/erase locus.\n");
  fprintf(stdout, "And...\n");
  fprintf(stdout, "Press 'd' key to show/erase dialog.\n");

  return true;
}

void SceneSolar2::update()
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

  if (kb.triggered(SDLK_l)) {
    locus_ = !locus_;
  }

  // 残像表示時はカメラ回転禁止にしてもよいが…
  // 数秒たてば復帰する残像効果が個人的にはむしろ面白い
  camera_.update();
  
  // 位置と速度を更新
  if (!pause_) {
    points_buffer_->update();
  }
}

void SceneSolar2::render()
{
  // pass 1(裏画面)
  fbo_[current_].bind();

  glClear(GL_COLOR_BUFFER_BIT);

  if (locus_) {
    static size_t count = 0;
    // 前フレームの描画結果をアルファブレンド
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    // 30フレームに一度だけシェーダーでアルファ値を下げる
    if (++count % 30 == 0) {
      locus_prog_.use();
      locus_prog_.set_uniform("Tex", 1 + (1 - current_)); // 前フレームの描画結果
      locus_prog_.set_uniform("factor", 0.002f);
      locus_prog_.set_uniform("alpha_threshold", 0.90f);
      count = 0;
    } else {
      quad_prog_.use();
      quad_prog_.set_uniform("Tex", 1 + (1 - current_)); // 前フレームの描画結果
    }
    quad_.render();
  }

  // points_buffer->update()の並列計算を同期待ち
  // 理屈上MemoryBarrier()はキリギリまで遅らせた方が余計なWaitが入らない筈
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // 現フレームの最新情報取得
  vec3 sun_pos, sun_vel, momentum;
  float en;
  points_buffer_->get_info(&sun_pos, &sun_vel, &momentum, &en);
  camera_.set_target(sun_pos);   // カメラに太陽を追尾させる
  
  if (imgui_) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Once);
    char title[40];
    snprintf(title, 40, "%d points solar-like simulation", point_num_);
    ImGui::Begin(title, &imgui_, IMGUI_SIMPLE_DIALOG_FLAGS);

    ImGui::Text("momentum: %+f, %+f, energy: %+14f", momentum.x, momentum.y, en);
    
    ImGui::Checkbox("show locus", &locus_);
    ImGui::SameLine(0, 180);
    
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
      points_buffer_->reset();
      glClear(GL_COLOR_BUFFER_BIT);
      // 太陽位置を再取得
      points_buffer_->get_info(&sun_pos, &sun_vel, &momentum, &en);
      camera_.set_target(sun_pos);   // カメラに太陽を追尾させる
    }
    ImGui::End();
  }
  
  // 点描画
  glEnable(GL_PROGRAM_POINT_SIZE);
  points_prog_.use();
  points_prog_.set_uniform("MVP", proj_ * camera_.view_matrix());
  points_buffer_->render_points();

  check_gl_error(__FILE__, __LINE__);

  fbo_[current_].bind(false);

  // pass 2 (裏画面を表画面に描画)
  glDisable(GL_BLEND);
  quad_prog_.use();
  quad_prog_.set_uniform("Tex", 1 + current_);
  quad_.render();

  // 軸描画
  render_axis(sun_pos, sun_vel);

  current_ = 1 - current_; // 裏画面交代
}

bool SceneSolar2::compile_and_link_shaders()
{
  using Names = std::vector<std::string>;

  // 描画用
  if (!points_prog_.build_program_from_files(Names{ "shader/solar2.vs", "shader/solar.fs" })) {
    return false;
  }
  if (!quad_prog_.build_program_from_files(Names{ "shader/quad.vs", "shader/quad.fs" })) {
    return false;
  }
  if (!locus_prog_.build_program_from_files(Names{ "shader/quad.vs", "shader/solar_locus.fs" })) {
    return false;
  }
  if (!axis_prog_.build_program_from_files(Names{ "shader/solar2_axis.vs", "shader/solar2_axis.fs" })) {
    return false;
  }

  // 計算用
  if (!vver_init_prog_.build_program_from_files(Names{ "shader/solar2_vver_init.cs" })) {
    return false;
  }
  if (!vver_prog_.build_program_from_files(Names{ "shader/solar2_vver.cs" })) {
    return false;
  }

  points_prog_.use();
  points_prog_.print_active_attribs();
  points_prog_.print_active_uniforms();

  return true;
}
