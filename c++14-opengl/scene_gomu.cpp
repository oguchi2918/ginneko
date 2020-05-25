#include <cstdio>
#include <vector>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"

#include "scene_gomu.hpp"
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

// 点+折れ線
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
  void calculate(size_t loop) const;
  unsigned point_num() const noexcept { return point_num_; }
  bool is_fix(int i) const { return fix_flags_[i]; }
  void trigger_fix(int i);
  void reset();
private:
  gl::Vao vao_;
  gl::VertexBuffer buffer_;

  gl::TexBuffer tex_buffer_;

  std::vector<bool> fix_flags_;

  const unsigned point_num_;
  const vec3 left_end_ = vec3(-0.9f, 0.f, 0.f);
  const vec3 right_end_ = vec3(0.9f, 0.f, 0.f);
};

PointBuffer::PointBuffer(unsigned n) : vao_(), buffer_(), tex_buffer_(GL_RGBA32F),
				       fix_flags_(n, false), point_num_(n)
{
  fix_flags_[0] = fix_flags_[n - 1] = true;
  
  buffer_.bind();
  glBufferData(GL_ARRAY_BUFFER, n * sizeof(vec4), nullptr, GL_DYNAMIC_COPY);

  vec4* pmapped = static_cast<vec4*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
  for (size_t i = 0; i < n; ++i) {
    // 線形補間
    float t = static_cast<float>(i) / (n - 1);
    pmapped[i] = vec4(left_end_ * (1.f - t) + right_end_ * t, fix_flags_[i] ? 0.f : 1.f);
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);

  tex_buffer_.bind();
  glBufferData(GL_TEXTURE_BUFFER, sizeof(vec4) * n, nullptr, GL_DYNAMIC_COPY);

  vao_.bind();
  buffer_.bind();
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
  vao_.bind(false);

  check_gl_error(__FILE__, __LINE__);
}

void PointBuffer::render(GLenum mode) const
{
  vao_.bind();
  glDrawArrays(mode, 0, point_num_);
}

int PointBuffer::pick(float x, float y, float dx, float dy) const
{
  buffer_.bind();
  vec4* pmapped = static_cast<vec4*>(glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY));
  for (size_t i = 0; i < point_num_; ++i) {
    if (fabs(pmapped[i].x - x) <= dx && fabs(pmapped[i].y - y) <= dy) {
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
  buffer_.bind();
  vec4 tmp(x, y, 0.f, w);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * i, sizeof(tmp), &tmp.x);
}

// i番目の節点を左ドラッグ終了時に固定するかどうか切り替える
void PointBuffer::trigger_fix(int i)
{
  assert(i >= 0 && i < static_cast<int>(point_num_));

  fix_flags_[i] = !fix_flags_[i];
  float w = fix_flags_[i] ? 0.f : 1.f;
  buffer_.bind();
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * i + sizeof(float) * 3, sizeof(w), &w);
}

void PointBuffer::calculate(size_t loop = 1) const
{
  vao_.bind();
  for (size_t i = 0; i < loop; ++i) {
    buffer_.bind();
    tex_buffer_.bind();

    // 頂点バッファからテクスチャバッファへ内容をコピー
    glCopyBufferSubData(GL_ARRAY_BUFFER, GL_TEXTURE_BUFFER, 0, 0, sizeof(vec4) * point_num_);

    // 頂点バッファをTransform Feedbackのターゲットに指定
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, buffer_.handle());

    // Transform Feedback開始
    glBeginTransformFeedback(GL_POINTS);
    
    tex_buffer_.bind_tex(1);

    glEnable(GL_RASTERIZER_DISCARD);
    glDrawArrays(GL_POINTS, 0, point_num_);
    glDisable(GL_RASTERIZER_DISCARD);

    glEndTransformFeedback();
    
    check_gl_error(__FILE__, __LINE__);
  }
}

void PointBuffer::reset()
{
  for (size_t i = 0; i < point_num_; ++i) {
    fix_flags_[i] = false;
  }
  fix_flags_[0] = fix_flags_[point_num_ - 1] = true;

  buffer_.bind();
  vec4* pmapped = static_cast<vec4*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
  for (size_t i = 1; i < point_num_ - 1; ++i) {
    // 線形補間
    float t = static_cast<float>(i) / (point_num_ - 1);
    pmapped[i] = vec4(left_end_ * (1.f - t) + right_end_ * t, 1.f);
  }
  pmapped[0] = vec4(left_end_, 0.f);
  pmapped[point_num_ - 1] = vec4(right_end_, 0.f);
  
  glUnmapBuffer(GL_ARRAY_BUFFER);

  check_gl_error(__FILE__, __LINE__);
}

SceneGomu::SceneGomu() : imgui_(false) {}
SceneGomu::~SceneGomu(){}

bool SceneGomu::init()
{
  if (!compile_and_link_shaders()) {
    return false;
  }

  // 20個の節点を持つ折れ線
  point_buffer_ = std::make_unique<PointBuffer>(20);

  prog_.use();

  glClearColor(1.f, 1.f, 1.f, 1.f);
  glPointSize(point_size_);
  
  fprintf(stdout, "\nYou can drag points with left mouse button.\n");
  fprintf(stdout, "Red points moves auto after dragging under power's influence.\n");
  fprintf(stdout, "Blue points are fixed after dragging.\n");
  fprintf(stdout, "Right mouse button click change Red/Blue status.\n");
  fprintf(stdout, "Press 'd' key to config dialog.\n");

  return true;
}

void SceneGomu::update()
{
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
      if (point_buffer_->is_fix(hit)) { // 画鋲で止めてある場合
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

  force_prog_.use();
  force_prog_.set_uniform("table", 1);
  point_buffer_->calculate(10);
}

void SceneGomu::render()
{
  glClear(GL_COLOR_BUFFER_BIT);

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

bool SceneGomu::compile_and_link_shaders()
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
  
  if (!force_prog_.compile_shader_from_file("shader/force.vs", ShaderType::VERTEX)) {
    fprintf(stderr, "Compiling vertex shader failed.\n%s\n", force_prog_.log().c_str());
    return false;
  }

  // setup transform feedback (must be before link)
  const char* output_names[] = { "position" };
  glTransformFeedbackVaryings(force_prog_.handle(), 1, output_names, GL_SEPARATE_ATTRIBS);

  if (!force_prog_.link()) {
    fprintf(stderr, "Linking shader program failed.\n%s\n", force_prog_.log().c_str());
    return false;
  }
  if (!force_prog_.valid()) {
    fprintf(stderr, "Validating program failed.\n%s\n", force_prog_.log().c_str());
    return false;
  }
  prog_.use();
  prog_.print_active_attribs();
  prog_.print_active_uniforms();

  return true;
}
