#include <cstdio>
#include <vector>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"

#include "scene_gomu4.hpp"
#include "defines.hpp"
#include "input.hpp"
#include "renderer.hpp"
#include "utils.hpp"

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

using namespace nekolib::renderer;

// 節点の状態(VBO用)
struct Point {
  alignas(16) vec4 position; // 節点位置(xyz) + 固定flag(w)
};

// 物理パラメーター(UBO用)
struct PhysicParams {
  alignas(4) uint point_num; // 節点数
  alignas(4) float dt; // タイムステップ
  alignas(4) float m; // 節点1個の質量
  alignas(4) float k; // ばね定数
  alignas(4) float c; // ばね減衰係数
};

// 点+折れ線描画
// 固定/自由の2値情報のみCPU側でも保持
// 1個だけ作ってunique_ptrに放り込むのでコピー&ムーブ不可の方針で.
class PointBuffer
{
public:
  PointBuffer(const PhysicParams*, Program&, Program&);
  ~PointBuffer() = default;

  PointBuffer(const PointBuffer&) = delete;
  PointBuffer& operator=(const PointBuffer&) = delete;
  PointBuffer(PointBuffer&&) = delete;
  PointBuffer& operator=(PointBuffer&&) = delete;

  void render(GLenum) const;
  void move(int i, float x, float y, bool fixed) const;
  void update();
  void next() { current_ = (current_ + 1) % 3; }
  unsigned point_num() const noexcept { return point_num_; }
  bool is_fix(int i) const noexcept { return fix_flags_[i]; }
  void trigger_fix(int i);
  void reset();
private:
  // verlet積分の場合pos(t)とpos(t-dt)からpos(t+dt)を計算するので
  // 入力用2個+出力用1個の計3個のバッファを計算シェーダーで使う
  gl::Vao vao_[3];
  gl::VertexBuffer buffer_[3];
  gl::UniformBuffer<PhysicParams> ubo_;

  std::vector<bool> fix_flags_;

  size_t current_; // 現在表示中のbuffer_ [0..2]
  const unsigned point_num_;
  const float dt_;
  const vec3 g_;

  // 計算シェーダー
  Program& update_prog_; // 位置更新担当
  Program& update_end_prog_; // 両端の位置更新担当
  
  const vec3 left_end_ = vec3(-0.9f, 0.5f, 0.f);
  const vec3 right_end_ = vec3(0.9f, 0.5f, 0.f);

  void init_force();
};

PointBuffer::PointBuffer(const PhysicParams* phsyc_param,
			 Program& update_prog, Program& update_end_prog)
  : ubo_(phsyc_param), fix_flags_(phsyc_param->point_num, false), current_(1),
    point_num_(phsyc_param->point_num), dt_(phsyc_param->dt), g_(0.f, -9.8f / point_num_, 0.f),
    update_prog_(update_prog), update_end_prog_(update_end_prog)
{
  // 両端だけ青(固定)
  fix_flags_[0] = fix_flags_[point_num_ - 1] = true;

  for (size_t i = 0; i < 3; ++i) {
    buffer_[i].bind();
    glBufferData(GL_ARRAY_BUFFER, point_num_ * sizeof(Point), nullptr, GL_DYNAMIC_DRAW);

    vao_[i].bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Point), BUFFER_OFFSET(0));
    vao_[i].bind(false);
  }

  buffer_[0].bind();
  Point* pmapped = static_cast<Point*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

  // pmapped[0..point_num_ - 1]を初期化
  for (size_t i = 0; i < point_num_; ++i) {
    // 位置
    float t = static_cast<float>(i) / (point_num_ - 1);
    pmapped[i].position = vec4(left_end_ * (1.f - t) + right_end_ * t, fix_flags_[i] ? 0.f : 1.f);
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);

  buffer_[1].bind();
  pmapped = static_cast<Point*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
  // 初速0なので同じ位置
  for (size_t i = 0; i < point_num_; ++i) {
    float t = static_cast<float>(i) / (point_num_  - 1);
    pmapped[i].position = vec4(left_end_ * (1.f - t) + right_end_ * t, fix_flags_[i] ? 0.f : 1.f);
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);

  // 物理パラメーターは全計算シェーダーで同じものを使用するのでUBOで設定
  ubo_.select(0);
  
  update_prog_.use();
  update_prog_.set_uniform_block("PhysicParams", 0);
  update_end_prog_.use();
  update_end_prog_.set_uniform_block("PhysicParams", 0);

  check_gl_error(__FILE__, __LINE__);
}

void PointBuffer::render(GLenum mode) const
{
  vao_[current_].bind();
  glDrawArrays(mode, 0, point_num_);
}

// i番目の節点を座標(x, y)に移動
// fixed = false の時力の影響を受ける : trueの時受けない
void PointBuffer::move(int i, float x, float y, bool fixed) const
{
  assert(i >= 0 && i < static_cast<int>(point_num_));
  
  buffer_[current_].bind();
  vec4 tmp(x, y, 0.f, fixed ? 0.f : 1.f);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(Point) * i, sizeof(tmp), &tmp.x);
}

// i番目の節点を左ドラッグ終了時に固定するかどうか切り替える
void PointBuffer::trigger_fix(int i)
{
  assert(i >= 0 && i < static_cast<int>(point_num_));

  fix_flags_[i] = !fix_flags_[i];
  // position.wは0.f/1.f切り替え
  // velocityは0.fクリア
  float tmp[] = {fix_flags_[i] ? 0.f : 1.f};
  buffer_[current_].bind();
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(Point) * i + sizeof(float) * 3, sizeof(tmp), &tmp);
}

// 更新用計算シェーダー起動
// 両端の節点とそれ以外の節点で別の計算シェーダーを使う
void PointBuffer::update()
{
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer_[(current_ - 1) % 3].handle());
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffer_[current_].handle());
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buffer_[(current_ + 1) % 3].handle());
  
  // 両端の節点用計算シェーダーを起動
  update_end_prog_.use();
  glDispatchCompute(1, 1, 1);

  update_prog_.use();

  // 残りの節点用計算シェーダーを起動
  glDispatchCompute(point_num_ - 2, 1, 1);

  check_gl_error(__FILE__, __LINE__);
}

// 節点を初期状態に戻す
void PointBuffer::reset()
{
  for (size_t i = 0; i < point_num_; ++i) {
    fix_flags_[i] = false;
  }
  fix_flags_[0] = fix_flags_[point_num_ - 1] = true;

  buffer_[0].bind();
  Point* pmapped = static_cast<Point*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
  for (size_t i = 0; i < point_num_; ++i) {
    // 線形補間
    float t = static_cast<float>(i) / (point_num_ - 1);
    pmapped[i].position = vec4(left_end_ * (1.f - t) + right_end_ * t, fix_flags_[i] ? 0.f : 1.f);
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);

  buffer_[1].bind();
  pmapped = static_cast<Point*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
  // 初速0、重力のみの条件で
  // x(t+dt) = x(t) + v(t)dt + f(t)dt^2/2m から
  // 初期配置の次の位置を計算する
  for (size_t i = 0; i < point_num_; ++i) {
    float t = static_cast<float>(i) / (point_num_  - 1);
    vec3 old = vec3(left_end_ * (1.f - t) + right_end_ * t);
    pmapped[i].position = vec4(old + 0.5f * dt_ * dt_ * g_, fix_flags_[i] ? 0.f : 1.f);
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);
  current_ = 1;

  check_gl_error(__FILE__, __LINE__);
}

// GeForce GT 640だとpoint_num_が100000でも滑らかに動く模様
// (重力小さくなり過ぎるけど)
SceneGomu4::SceneGomu4() : point_num_(100), imgui_(false) {}
SceneGomu4::~SceneGomu4(){}

struct PixelInfo {
  unsigned point_id_;

  PixelInfo() : point_id_(0) {}
};

PixelInfo SceneGomu4::read_pixel(unsigned x, unsigned y)
{
  fbo_.bind();
  glReadBuffer(GL_COLOR_ATTACHMENT1);
  PixelInfo pixel;
  glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &pixel);
  glReadBuffer(GL_NONE);
  fbo_.bind(false);

  check_gl_error(__FILE__, __LINE__);

  return pixel;
}

bool SceneGomu4::setup_fbo()
{
  fbo_.bind();

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, render_tex_.handle(), 0);

  pick_.bind();
  glRenderbufferStorage(GL_RENDERBUFFER, GL_R32UI, ScreenManager::width(), ScreenManager::height());
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, pick_.handle());
  //  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, pick_tex_.handle(), 0);

  if (check_fbo_status(__FILE__, __LINE__)) {
    return false;
  }

  GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
  glDrawBuffers(2, draw_buffers);

  fbo_.bind(false);

  return true;
}

bool SceneGomu4::init()
{
  if (!compile_and_link_shaders()) {
    return false;
  }

  cur_ = nekolib::clock::Clock::create(0.f);
  prev_ = cur_.snapshot();

  quad_ = Quad::create(0.f, 0.f, 1.f, 1.f);
  render_tex_ = Texture::create(ScreenManager::width(), ScreenManager::height(), TextureFormat::RGB8);
  render_tex_.bind(1);

  if (!setup_fbo()) {
    return false;
  }

  // 物理パラメーター
  const PhysicParams physic_param =
    { point_num_,
      1.f / 60,
      1.f,
      250.f,
      25.f,
    };

  // 節点+折れ線
  point_buffer_ = std::make_unique<PointBuffer>(&physic_param,
						update_prog_, update_end_prog_);

  point_prog_.use();
  point_prog_.set_uniform("color_move", vec4(1.f, 0.f, 0.f, 1.f));
  point_prog_.set_uniform("color_fix", vec4(0.f, 0.f, 1.f, 1.f));
  
  glPointSize(point_size_);
  
  fprintf(stdout, "\nYou can drag a point with left mouse button.\n");
  fprintf(stdout, "Red points moves auto after dragging under force's influence.\n");
  fprintf(stdout, "Blue points are fixed to that position after dragging.\n");
  fprintf(stdout, "Click a point with right mouse button to switch red/blue.\n");
  fprintf(stdout, "Press 'd' key to show reset dialog.\n");

  return true;
}

void SceneGomu4::update()
{
  // float dt = nekolib::clock::Clock::calc_delta_seconds(cur_, prev_);
  // prev_ = cur_.snapshot();
  // dtが大きいと計算結果が不正になるので計算シェーダーに渡すのは止め

  using namespace nekolib::input;
  Mouse m = nekolib::input::Manager::instance().mouse();
  Keyboard kb = nekolib::input::Manager::instance().keyboard();

  // imgui表示中は入力は全てそちらへ
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

  static int hit = -1;
  int cx = nekolib::renderer::ScreenManager::width() / 2;
  int cy = nekolib::renderer::ScreenManager::height() / 2;
  float cx_inv = 1.f / cx;
  float cy_inv = 1.f / cy;
  float fx = static_cast<float>(m.x() - cx) * cx_inv;
  float fy = static_cast<float>(cy - m.y()) * cy_inv;

  if (m.triggered(Mouse::Button::LEFT)) {
    PixelInfo pixel = read_pixel(m.x(), nekolib::renderer::ScreenManager::height() - m.y() - 1);
    //    fprintf(stderr, "%x\n", pixel.point_id_);
    hit = static_cast<int>(pixel.point_id_);
  } else if (m.pushed(Mouse::Button::LEFT)) { // 左ドラッグ中
    if (hit >= 0) {
      point_buffer_->move(hit, fx, fy, true);
    }
  } else {
    if (hit >= 0) { // 左ドラッグ終了
      if (point_buffer_->is_fix(hit)) {
	// 端点は力の計算の影響を受けない(画鋲で止めてあるという設定)
	point_buffer_->move(hit, fx, fy, true);
      } else {
	point_buffer_->move(hit, fx, fy, false);
      }
      hit = -1;
    }
  }
  
  if (m.triggered(Mouse::Button::RIGHT)) {
    PixelInfo pixel = read_pixel(m.x(), nekolib::renderer::ScreenManager::height() - m.y() - 1);
    if (pixel.point_id_ != clear_pick_) {
      point_buffer_->trigger_fix(pixel.point_id_);
    }
  }

  // 力の影響を計算して位置を更新
  point_buffer_->update();
  point_buffer_->next();
}

void SceneGomu4::render()
{
  // pass 1
  fbo_.bind();

  // 描画と3D pickのクリア
  glClearBufferfv(GL_COLOR, 0, &clear_color_.x);
  glClearBufferuiv(GL_COLOR, 1, &clear_pick_);

  // point_buffer->calculate()の並列計算を同期待ち
  // 理屈上MemoryBarrier()はキリギリまで遅らせた方が余計なWaitが入らない筈
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  
  if (imgui_) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Once);
    ImGui::Begin("config", &imgui_, IMGUI_SIMPLE_DIALOG_FLAGS);
    
    if (ImGui::Button("Reset")) {
      point_buffer_->reset();
    }
    ImGui::End();
  }

  line_prog_.use();
  point_buffer_->render(GL_LINE_STRIP);
  point_prog_.use();
  point_buffer_->render(GL_POINTS);

  check_gl_error(__FILE__, __LINE__);

  fbo_.bind(false);

  // pass 2
  quad_prog_.use();
  quad_prog_.set_uniform("Tex", 1);
  quad_.render();
}

bool SceneGomu4::compile_and_link_shaders()
{
  using Names = std::vector<std::string>;

  // 描画用
  if (!point_prog_.build_program_from_files(Names{ "shader/gomu2.vs", "shader/gomu2.fs" })) {
    return false;
  }

  if (!line_prog_.build_program_from_files(Names{ "shader/gomu2_line.vs", "shader/gomu2_line.fs" })) {
    return false;
  }

  if (!quad_prog_.build_program_from_files(Names{ "shader/quad.vs", "shader/quad.fs" })) {
    return false;
  }

  // 計算用
  if (!update_prog_.build_program_from_files(Names{ "shader/gomu_ver.cs" })) {
    return false;
  }
  
  if (!update_end_prog_.build_program_from_files(Names{ "shader/gomu_ver_end.cs" })) {
    return false;
  }

  point_prog_.use();
  point_prog_.print_active_attribs();
  point_prog_.print_active_uniforms();

  return true;
}
