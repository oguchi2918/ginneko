#include <cstdio>
#include <vector>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"

#include "scene_gomu2.hpp"
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

struct Point {
  vec4 position;
  vec4 velocity;
};

// 点+折れ線
// 固定/自由の2値情報のみCPU側でも保持
// 1個だけ作ってunique_ptrに放り込むのでコピー&ムーブ不可の方針で.
class PointBuffer
{
public:
  PointBuffer(unsigned int n);
  ~PointBuffer() = default;

  PointBuffer(const PointBuffer&) = delete;
  PointBuffer& operator=(const PointBuffer&) = delete;
  PointBuffer(PointBuffer&&) = delete;
  PointBuffer& operator=(PointBuffer&&) = delete;

  void render(GLenum) const;
  int pick(float x, float y, float dx, float dy) const;
  void move(int i, float x, float y, float w) const;
  void end_calculate();
  void calculate(nekolib::renderer::Program&, nekolib::renderer::Program&);
  unsigned point_num() const noexcept { return point_num_; }
  bool is_fix(int i) const noexcept { return fix_flags_[i]; }
  void trigger_fix(int i);
  void reset();
private:
  gl::Vao vao_[2];
  gl::VertexBuffer buffer_[2];

  std::vector<bool> fix_flags_;

  size_t current_; // どちらのbuffer_を表示対象とするか(0 or 1)
  
  const unsigned point_num_;
  const vec3 left_end_ = vec3(-0.9f, 0.5f, 0.f);
  const vec3 right_end_ = vec3(0.9f, 0.5f, 0.f);
};

PointBuffer::PointBuffer(unsigned n) : fix_flags_(n, false), current_(0), point_num_(n)
{
  // 両端だけ青(固定)
  fix_flags_[0] = fix_flags_[n - 1] = true;

  for (size_t i = 0; i < 2; ++i) {
    buffer_[i].bind();
    glBufferData(GL_ARRAY_BUFFER, (n + 2) * sizeof(Point), nullptr, GL_STATIC_DRAW);

    vao_[i].bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Point), BUFFER_OFFSET(0));
    vao_[i].bind(false);
  }

  buffer_[0].bind();
  Point* pmapped = static_cast<Point*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
  // pmapped[0..n - 1]にデータを書き込む
  for (size_t i = 0; i < n; ++i) {
    // 線形補間
    float t = static_cast<float>(i) / (n - 1);
    pmapped[i].position = vec4(left_end_ * (1.f - t) + right_end_ * t, fix_flags_[i] ? 0.f : 1.f);
    pmapped[i].velocity = vec4(0.f, 0.f, 0.f, 0.f);
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);

  check_gl_error(__FILE__, __LINE__);
}

void PointBuffer::render(GLenum mode) const
{
  vao_[current_].bind();
  glDrawArrays(mode, 0, point_num_);
}

int PointBuffer::pick(float x, float y, float dx, float dy) const
{
  buffer_[current_].bind();
  Point* pmapped = static_cast<Point*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));
  for (size_t i = 0; i < point_num_; ++i) {
    if (fabs(pmapped[i].position.x - x) <= dx && fabs(pmapped[i].position.y - y) <= dy) {
      glUnmapBuffer(GL_ARRAY_BUFFER);
      return i;
    }
  }

  glUnmapBuffer(GL_ARRAY_BUFFER);

  // can't found
  return -1;
}

// i番目の節点を座標(x, y)に移動
// w = 1.f の時力の影響を受ける : w = 0.fの時受けない
void PointBuffer::move(int i, float x, float y, float w) const
{
  assert(i >= 0 && i < static_cast<int>(point_num_));
  
  buffer_[current_].bind();
  vec4 tmp(x, y, 0.f, w);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(Point) * i, sizeof(tmp), &tmp.x);
}

// i番目の節点を左ドラッグ終了時に固定するかどうか切り替える
void PointBuffer::trigger_fix(int i)
{
  assert(i >= 0 && i < static_cast<int>(point_num_));

  fix_flags_[i] = !fix_flags_[i];
  // position.wは0.f/1.f切り替え
  // velocityは0.fクリア
  float tmp[] = {fix_flags_[i] ? 0.f : 1.f, 0.f, 0.f, 0.f, 0.f};
  buffer_[current_].bind();
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(Point) * i + sizeof(float) * 3, sizeof(tmp), &tmp);
}

void PointBuffer::calculate(nekolib::renderer::Program& comp_prog,
			    nekolib::renderer::Program& comp_end_prog)
{
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer_[current_].handle());
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffer_[1 - current_].handle());
  
  comp_end_prog.use();

  // 両端の節点の力の計算シェーダーを起動
  glDispatchCompute(1, 1, 1);

  // 多分要らない
  //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  
  comp_prog.use();  

  // 残りの節点の力の計算シェーダーを起動
  glDispatchCompute(point_num_ - 2, 1, 1);

  check_gl_error(__FILE__, __LINE__);

  // バッファ交代  
  current_ = 1 - current_;
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
    pmapped[i].velocity = vec4(0.f, 0.f, 0.f, 0.f);
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);

  current_ = 0;

  check_gl_error(__FILE__, __LINE__);
}

// GeForce GT 640だとpoint_num_が100000でも滑らかに動く模様
// (重力小さくなり過ぎるけど)
SceneGomu2::SceneGomu2() : point_num_(100), imgui_(false) {}
SceneGomu2::~SceneGomu2(){}

bool SceneGomu2::init()
{
  if (!compile_and_link_shaders()) {
    return false;
  }

  cur_ = nekolib::clock::Clock::create(0.f);
  prev_ = cur_.snapshot();
  
  // 節点+折れ線
  point_buffer_ = std::make_unique<PointBuffer>(point_num_);

  comp_end_prog_.use();
  comp_end_prog_.set_uniform("point_num", point_num_);
  comp_prog_.use();
  comp_prog_.set_uniform("point_num", point_num_);

  glClearColor(1.f, 1.f, 1.f, 1.f);
  glPointSize(point_size_);
  
  fprintf(stdout, "\nYou can drag a point with left mouse button.\n");
  fprintf(stdout, "Red points moves auto after dragging under force's influence.\n");
  fprintf(stdout, "Blue points are fixed to that position after dragging.\n");
  fprintf(stdout, "Click a point with right mouse button to switch red/blue.\n");
  fprintf(stdout, "Press 'd' key to show reset dialog.\n");

  return true;
}

void SceneGomu2::update()
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
  float fdx = static_cast<float>(point_size_) * cx_inv * 0.5;
  float fdy = static_cast<float>(point_size_) * cy_inv * 0.5;

  if (m.triggered(Mouse::Button::LEFT)) {
    hit = point_buffer_->pick(fx, fy, fdx, fdy);
  } else if (m.pushed(Mouse::Button::LEFT)) { // 左ドラッグ中
    if (hit >= 0) {
      point_buffer_->move(hit, fx, fy, 0.f);
    }
  } else {
    if (hit >= 0) { // 左ドラッグ終了
      if (point_buffer_->is_fix(hit)) {
	// 端点は力の計算の影響を受けない(画鋲で止めてあるという設定)
	point_buffer_->move(hit, fx, fy, 0.f);
      } else {
	point_buffer_->move(hit, fx, fy, 1.f);
      }
      hit = -1;
    }
  }
  
  if (m.triggered(Mouse::Button::RIGHT)) {
    int hit_r = point_buffer_->pick(fx, fy, fdx, fdy);
    if (hit_r >= 0) {
      point_buffer_->trigger_fix(hit_r);
    }
  }

  // 力の影響を計算して位置と速度を更新
  point_buffer_->calculate(comp_prog_, comp_end_prog_);
}

void SceneGomu2::render()
{
  glClear(GL_COLOR_BUFFER_BIT);

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

  prog_.use();
  prog_.set_uniform("color_move", vec4(0.f, 0.f, 0.f, 1.f));
  prog_.set_uniform("color_fix", vec4(0.f, 0.f, 0.f, 1.f));
  point_buffer_->render(GL_LINE_STRIP);
  prog_.set_uniform("color_move", vec4(1.f, 0.f, 0.f, 1.f));
  prog_.set_uniform("color_fix", vec4(0.f, 0.f, 1.f, 1.f));  
  point_buffer_->render(GL_POINTS);

  check_gl_error(__FILE__, __LINE__);
}

bool SceneGomu2::compile_and_link_shaders()
{
  if (!prog_.compile_shader_from_file("shader/gomu.vs", ShaderType::VERTEX)) {
    fprintf(stderr, "Compiling vertex shader failed.\n%s\n", prog_.log().c_str());
    return false;
  }
  if (!prog_.compile_shader_from_file("shader/gomu.fs", ShaderType::FRAGMENT)) {
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
  
  if (!comp_prog_.compile_shader_from_file("shader/gomu.cs", ShaderType::COMPUTE)) {
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
  
  if (!comp_end_prog_.compile_shader_from_file("shader/gomu_end.cs", ShaderType::COMPUTE)) {
    fprintf(stderr, "Compiling compute shader failed.\n%s\n", comp_end_prog_.log().c_str());
    return false;
  }
  
  if (!comp_end_prog_.link()) {
    fprintf(stderr, "Linking shader program failed.\n%s\n", comp_end_prog_.log().c_str());
    return false;
  }
  
  if (!comp_end_prog_.valid()) {
    fprintf(stderr, "Validating program failed.\n%s\n", comp_end_prog_.log().c_str());
    return false;
  }
  
  prog_.use();
  prog_.print_active_attribs();
  prog_.print_active_uniforms();

  return true;
}
