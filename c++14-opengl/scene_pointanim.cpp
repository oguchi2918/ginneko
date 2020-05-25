#include <vector>
#include <random>
#include <cstdio>
#include <cmath>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"

#include "scene_pointanim.hpp"
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

static const size_t POINTS = 100000;
static const float CYCLE = 5.f;

bool ScenePointAnim::init()
{
  if (!compile_and_link_shaders()) {
    return false;
  }

  //  glEnable(GL_DEPTH_TEST);

  cur_ = nekolib::clock::Clock::create(0.f);
  start_ = cur_.snapshot();
  
  view_ = glm::lookAt(vec3(0.f, 0.f, 7.f), vec3(0.f, 0.f, 0.f), vec3(0.f, 1.f, 0.f));
  proj_ = glm::perspective(glm::radians(30.f), ScreenManager::aspectf(), 5.f, 9.f);

  disseminate(POINTS);

  prog_.use();

  glClearColor(0.f, 0.1f, 0.3f, 1.f);

  fprintf(stdout, "Press 'd' key to show color dialog.\n");
  
  return true;
}

void ScenePointAnim::update_rotation(int x, int y)
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

void ScenePointAnim::disseminate(size_t point_num)
{
  using Point = GLfloat[3];

  buffer_.bind();
  glBufferData(GL_ARRAY_BUFFER, sizeof(Point) * point_num, nullptr, GL_STATIC_DRAW);
  Point* point = static_cast<Point*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

  std::random_device rd;
  std::mt19937 mt(rd()); // 乱数生成
  std::uniform_real_distribution<float> dist(0.f, 1.f); // [0.f, 1.f)一様分布の乱数
  /* 頂点位置を生成 */
  for (size_t i = 0; i < point_num; ++i) {
    float r = sqrt(2.f * dist(mt)); // y=x^2/2 の逆関数
    float t = glm::two_pi<float>() * dist(mt);
    (*point)[0] = r * cos(t);
    (*point)[1] = r * sin(t);
    (*point)[2] = dist(mt);
    ++point;
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);

  vao_.bind();
  buffer_.bind();
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
  vao_.bind(false);
}

void ScenePointAnim::update()
{
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

void ScenePointAnim::render()
{
  static vec3 bg(0.f, 0.1, 0.3f);
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
  prog_.set_uniform("point_color", point_color);

  model_ = glm::translate(mat4(1.f), vec3(0.f, 0.f, -1.f));
  model_ = glm::scale(model_, vec3(1.f, 1.f, 2.f));
  model_ = glm::mat4(rot_) * model_;
  
  prog_.set_uniform("MVP", proj_ * view_ * model_);

  float delta = nekolib::clock::Clock::calc_delta_seconds(cur_, start_); // 経過時間(秒)
  prog_.set_uniform("elapsed_time", delta / CYCLE);

  vao_.bind();
  glDrawArrays(GL_POINTS, 0, POINTS);

  check_gl_error(__FILE__, __LINE__);
}

bool ScenePointAnim::compile_and_link_shaders()
{
  if (!prog_.compile_shader_from_file("shader/points.vs", ShaderType::VERTEX)) {
    fprintf(stderr, "Compiling vertex shader failed.\n%s\n", prog_.log().c_str());
    return false;
  }
  if (!prog_.compile_shader_from_file("shader/points.fs", ShaderType::FRAGMENT)) {
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

  prog_.use();
  prog_.print_active_attribs();
  prog_.print_active_uniforms();

  return true;
}
