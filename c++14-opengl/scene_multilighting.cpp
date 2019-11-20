#include <cstdio>
#include <vector>
#include <array>
#include <utility>
#include <algorithm>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"

#include "scene_multilighting.hpp"
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

bool SceneMultiLighting::init()
{
  if (!compile_and_link_shaders()) {
    return false;
  }

  cube_ = Cube::create();

  cur_ = nekolib::clock::Clock::create(0.f);
  prev_ = cur_.snapshot();
  
  diffuse_map_ = Texture::create("texture/container2.png");
  specular_map_ = Texture::create("texture/container2_specular.png");
  if (!diffuse_map_ || !specular_map_) {
    fprintf(stderr, "load texture image failed.\n");
    return false;
  }

  // material properties
  prog_.set_uniform("material.diffuse", 0);
  prog_.set_uniform("material.specular", 1);
  prog_.set_uniform("material.shininess", 32.f);

  diffuse_map_.bind(0);
  specular_map_.bind(1);

  // light properties
  // directional light
  prog_.set_uniform("dir_light.ambient", 0.05f, 0.05f, 0.05f);
  prog_.set_uniform("dir_light.diffuse", 0.4f, 0.4f, 0.4f);
  prog_.set_uniform("dir_light.specular", 0.5f, 0.5f, 0.5f);
  // point light1
  prog_.set_uniform("point_lights[0].ambient", 0.05f, 0.05f, 0.05f);
  prog_.set_uniform("point_lights[0].diffuse", 0.8f, 0.8f, 0.8f);
  prog_.set_uniform("point_lights[0].specular", 1.0f, 1.0f, 1.0f);
  prog_.set_uniform("point_lights[0].constant", 1.0f);
  prog_.set_uniform("point_lights[0].linear", 0.09f);
  prog_.set_uniform("point_lights[0].quadratic", 0.032f);
  // point light 2
  prog_.set_uniform("point_lights[1].ambient", 0.05f, 0.05f, 0.05f);
  prog_.set_uniform("point_lights[1].diffuse", 0.8f, 0.8f, 0.8f);
  prog_.set_uniform("point_lights[1].specular", 1.0f, 1.0f, 1.0f);
  prog_.set_uniform("point_lights[1].constant", 1.0f);
  prog_.set_uniform("point_lights[1].linear", 0.09f);
  prog_.set_uniform("point_lights[1].quadratic", 0.032f);
  // point light 3
  prog_.set_uniform("point_lights[2].ambient", 0.05f, 0.05f, 0.05f);
  prog_.set_uniform("point_lights[2].diffuse", 0.8f, 0.8f, 0.8f);
  prog_.set_uniform("point_lights[2].specular", 1.0f, 1.0f, 1.0f);
  prog_.set_uniform("point_lights[2].constant", 1.0f);
  prog_.set_uniform("point_lights[2].linear", 0.09f);
  prog_.set_uniform("point_lights[2].quadratic", 0.032f);
  // point light 4
  prog_.set_uniform("point_lights[3].ambient", 0.05f, 0.05f, 0.05f);
  prog_.set_uniform("point_lights[3].diffuse", 0.8f, 0.8f, 0.8f);
  prog_.set_uniform("point_lights[3].specular", 1.0f, 1.0f, 1.0f);
  prog_.set_uniform("point_lights[3].constant", 1.0f);
  prog_.set_uniform("point_lights[3].linear", 0.09f);
  prog_.set_uniform("point_lights[3].quadratic", 0.032f);
  // spotlight
  prog_.set_uniform("spot_light.position", 0.f, 0.f, 0.f); // on camera
  prog_.set_uniform("spot_light.direction", 0.f, 0.f, -1.f);
  prog_.set_uniform("spot_light.ambient", 0.f, 0.f, 0.f);
  prog_.set_uniform("spot_light.diffuse", 1.f, 1.f, 1.f);
  prog_.set_uniform("spot_light.specular", 1.f, 1.f, 1.f);
  prog_.set_uniform("spot_light.constant", 1.f);
  prog_.set_uniform("spot_light.linear", 0.09f);
  prog_.set_uniform("spot_light.quadratic", 0.032f);
  prog_.set_uniform("spot_light.cutoff", glm::cos(glm::radians(12.5f)));
  prog_.set_uniform("spot_light.outer_cutoff", glm::cos(glm::radians(15.f)));

  lamp_prog_.use();
  lamp_prog_.set_uniform("light_color", 1.f, 1.f, 1.f);

  glEnable(GL_DEPTH_TEST);
  glClearColor(0.1f, 0.1f, 0.1f, 1.f);

  fprintf(stderr, "Press 'd' key to configure color properties.\n");

  return true;
}

void SceneMultiLighting::update()
{
  float delta = nekolib::clock::Clock::calc_delta_seconds(cur_, prev_);
  prev_ = cur_.snapshot();
  
  using namespace nekolib::input;
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

  camera_.update(delta);
}

void SceneMultiLighting::render()
{
  static vec3 bg(0.1f, 0.1f, 0.1f);
  static vec3 dir_amb(0.05f, 0.05f, 0.05f);
  static vec3 dir_dif_spe(0.5f, 0.5f, 0.5f);

  static vec3 point_colors[point_lights_num_] =
    { vec3(0.9f, 0.9f, 0.9f),
      vec3(0.9f, 0.9f, 0.9f),
      vec3(0.9f, 0.9f, 0.9f),
      vec3(0.9f, 0.9f, 0.9f) };
  static int point_cover_distance = 50;
  static vec3 flash_color(1.f, 1.f, 1.f);

  static int flash_cover_distance = 50;
  static float flash_cutoff = 12.5f;
  static float flash_edge_delta = 2.5f;

  if (imgui_) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Once);
    ImGui::Begin("configure", &imgui_, IMGUI_SIMPLE_DIALOG_FLAGS);

    if (ImGui::CollapsingHeader("Directional light (and background)")) {
      ImGui::ColorEdit3("ambient", &dir_amb.x);
      ImGui::ColorEdit3("diffuse & specular", &dir_dif_spe.x);
      ImGui::ColorEdit3("background", &bg.x);
    }

    if (ImGui::CollapsingHeader("Point lights")) {
      static bool b(false);
      ImGui::Checkbox("sync all lights color", &b);
      if (!b) {
	ImGui::ColorEdit3("light 0 color", &point_colors[0].x);
	ImGui::ColorEdit3("light 1 color", &point_colors[1].x);
	ImGui::ColorEdit3("light 2 color", &point_colors[2].x);
	ImGui::ColorEdit3("light 3 color", &point_colors[3].x);
      } else {
	ImGui::ColorEdit3("light 0 color", &point_colors[0].x);
	point_colors[3] = point_colors[2] = point_colors[1] = point_colors[0];
      }
      ImGui::NewLine();
      ImGui::SliderInt("light cover radius", &point_cover_distance, 5, 300);
    }

    if (ImGui::CollapsingHeader("Flash light")) {
      ImGui::ColorEdit3("color (diffuse and specular)", &flash_color.x);
      ImGui::NewLine();
      ImGui::SliderInt("light cover radius", &flash_cover_distance, 5, 300);
      ImGui::SliderFloat("cutoff degree", &flash_cutoff, 0.f, 60.f);
      ImGui::SliderFloat("edge degree delta", &flash_edge_delta, 0.1f, 15.f);
    }

    ImGui::End();
  }

  glClearColor(bg.x, bg.y, bg.z, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  view_ = camera_.view_matrix();
  proj_ = glm::perspective(glm::radians(camera_.zoom()), ScreenManager::aspectf(), 0.1f, 100.f);

  prog_.use();
  
  // update directional light properties...
  const vec3 light_dir(-0.2f, -1.f, -0.3f);
  prog_.set_uniform("dir_light.direction", vec3(view_ * vec4(light_dir, 0.f)));
  prog_.set_uniform("dir_light.ambient", dir_amb);
  prog_.set_uniform("dir_light.diffuse", dir_dif_spe);
  prog_.set_uniform("dir_light.specular", dir_dif_spe);

  
  // positions of all point lights
  vec3 light_positions[] = { vec3(0.7f, 0.2f, 2.f),
			     vec3(2.3f, -3.3f, -4.f),
			     vec3(-4.f, 2.f, -12.f),
			     vec3(0.f, 0.f, -3.f) };
  vec3 point_attenuation_param = calc_attenuation_parm(point_cover_distance);

  // update point lights properties...
  for (int i = 0; i < point_lights_num_; ++i) {
    char buf[64];
    auto n = snprintf(buf, 64, "point_lights[%d].position", i);
    assert(n < 64);
    prog_.set_uniform(buf, vec3(view_ * vec4(light_positions[i], 1.f)));

    n = snprintf(buf, 64, "point_lights[%d].constant", i);
    assert(n < 64);
    prog_.set_uniform(buf, point_attenuation_param.x);
    n = snprintf(buf, 64, "point_lights[%d].linear", i);
    assert(n < 64);
    prog_.set_uniform(buf, point_attenuation_param.y);
    n = snprintf(buf, 64, "point_lights[%d].quadratic", i);
    assert(n < 64);
    prog_.set_uniform(buf, point_attenuation_param.z);
    
    n = snprintf(buf, 64, "point_lights[%d].ambient", i);
    assert(n < 64);
    prog_.set_uniform(buf, 0.1f * point_colors[i]);
    n = snprintf(buf, 64, "point_lights[%d].diffuse", i);
    assert(n < 64);
    prog_.set_uniform(buf, point_colors[i]);
    n = snprintf(buf, 64, "point_lights[%d].specular", i);
    assert(n < 64);
    prog_.set_uniform(buf, point_colors[i]);
  }

  // update flash light properties...
  vec3 flash_attenuation_param = calc_attenuation_parm(flash_cover_distance);
  prog_.set_uniform("spot_light.cutoff", glm::cos(glm::radians(flash_cutoff)));
  prog_.set_uniform("spot_light.outer_cutoff", glm::cos(glm::radians(flash_cutoff + flash_edge_delta)));
  prog_.set_uniform("spot_light.constant", flash_attenuation_param.x);
  prog_.set_uniform("spot_light.linear", flash_attenuation_param.y);
  prog_.set_uniform("spot_light.quadratic", flash_attenuation_param.z);
  prog_.set_uniform("spot_light.diffuse", flash_color);
  prog_.set_uniform("spot_light.specular", flash_color);
  
  // positions of all containers
  vec3 cube_positions[] = { vec3(0.f, 0.f, 0.f),
			    vec3(2.f, 5.f, -15.f),
			    vec3(-1.5f, -2.2f, -2.5f),
			    vec3(-3.8f, -2.f, -12.3f),
			    vec3(2.4f, -0.4f, -3.5f),
			    vec3(-1.7f, 3.f, -7.5f),
			    vec3(1.3f, -2.f, -2.5f),
			    vec3(1.5f, 2.f, -2.5f),
			    vec3(1.5f, 0.2f, -1.5f),
			    vec3(-1.3f, 1.f, -1.5f) };

  for (int i = 0; i < 10; ++i) {
    model_ = glm::translate(mat4(1.f), cube_positions[i]);
    model_ = glm::rotate(model_, glm::radians(20.f * i), vec3(1.f, 0.3f, 0.5f));

    mat4 mv = view_ * model_;
  
    prog_.set_uniform("MVP", proj_ * mv);
    prog_.set_uniform("modelview", mv);
    prog_.set_uniform("normalmat", glm::transpose(glm::inverse(mat3(mv))));

    cube_.render();
  }

  check_gl_error(__FILE__, __LINE__);

  lamp_prog_.use();
  for (int i = 0; i < 4; ++i) {
    model_ = glm::translate(mat4(1.f), light_positions[i]);
    model_ = glm::scale(model_, vec3(0.2f));
    lamp_prog_.set_uniform("MVP", proj_ * view_ * model_);
    lamp_prog_.set_uniform("light_color", point_colors[i]);
    cube_.render();
  }

  check_gl_error(__FILE__, __LINE__);
}

vec3 SceneMultiLighting::calc_attenuation_parm(int distance)
{
  using KandV = std::pair<int, vec3>; // key and value
  std::array<KandV, 10> table = { std::make_pair(7, vec3(1.f, 0.7f, 1.8f)),
				  std::make_pair(13, vec3(1.f, 0.35f, 0.44f)),
				  std::make_pair(20, vec3(1.f, 0.22f, 0.20f)),
				  std::make_pair(32, vec3(1.f, 0.14f, 0.07f)),
				  std::make_pair(50, vec3(1.f, 0.09f, 0.032f)),
				  std::make_pair(65, vec3(1.f, 0.07f, 0.017f)),
				  std::make_pair(100, vec3(1.f, 0.045f, 0.0075f)),
				  std::make_pair(160, vec3(1.f, 0.027f, 0.0028f)),
				  std::make_pair(200, vec3(1.f, 0.022f, 0.0019f)),
				  std::make_pair(325, vec3(1.f, 0.014f, 0.0007f)), };

		    
  auto it = std::lower_bound(table.begin(), table.end(), distance,
			     [](const KandV& kv, int n) { return kv.first < n; });

  if (it == table.begin()) {
    return table[0].second;
  } else if (it == table.end()) {
    return table[9].second;
  } else {
    const KandV& a(*it), b(*(++it));
    // 線形補完とかしてみる
    return glm::mix(a.second, b.second, static_cast<float>(distance - a.first) / (b.first - a.first));
  }
}

bool SceneMultiLighting::compile_and_link_shaders()
{
  if (!prog_.compile_shader_from_file("shader/multiple_lights.vs", ShaderType::VERTEX)) {
    fprintf(stderr, "Compiling vertex shader failed.\n%s\n", prog_.log().c_str());
    return false;
  }
  if (!prog_.compile_shader_from_file("shader/multiple_lights.fs", ShaderType::FRAGMENT)) {
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

  if (!lamp_prog_.compile_shader_from_file("shader/lamp.vs", ShaderType::VERTEX)) {
    fprintf(stderr, "Compiling vertex shader failed.\n%s\n", lamp_prog_.log().c_str());
    return false;
  }
  if (!lamp_prog_.compile_shader_from_file("shader/lamp.fs", ShaderType::FRAGMENT)) {
    fprintf(stderr, "Compiling fragment shader failed.\n%s\n", lamp_prog_.log().c_str());
    return false;
  }
  if (!lamp_prog_.link()) {
    fprintf(stderr, "Linking shader program failed.\n%s\n", lamp_prog_.log().c_str());
    return false;
  }
  if (!lamp_prog_.valid()) {
    fprintf(stderr, "Validating program failed.\n%s\n", lamp_prog_.log().c_str());
    return false;
  }
  
  prog_.use();
  prog_.print_active_attribs();
  prog_.print_active_uniforms();

  return true;
}
