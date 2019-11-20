#include <cstdio>
#include <vector>

#include <iostream>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"

#include "scene_cameratest.hpp"
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

bool SceneCameraTest::init()
{
  if (!compile_and_link_shaders()) {
    return false;
  }

  glClearColor(0.0f, 0.0f, 1.f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  cube_ = Cube::create();

  cur_ = nekolib::clock::Clock::create(0.f);
  prev_ = cur_.snapshot();
  
  container_tex_ = Texture::create("texture/container.jpg");
  face_tex_ = Texture::create("texture/awesomeface.png");
  if (!container_tex_ || !face_tex_) {
    fprintf(stderr, "load texture image failed.\n");
    return false;
  }

  prog_.set_uniform("ContainerTex", 0);
  prog_.set_uniform("FaceTex", 1);

  glClearColor(0.2f, 0.3f, 0.3f, 1.f);
  container_tex_.bind(0);
  face_tex_.bind(1);

  fprintf(stdout, "Press 'd' key to show selecting camera dialog.\n");
  fprintf(stdout, "With QuatCamera, you can rotate or move camera with dragging mouse.\n");
  fprintf(stdout, "With left mouse button, with right mouse button, with both buttons.\n");
  fprintf(stdout, "With EulerCamera, you can move camera with pushing arrow keys.\n");
  fprintf(stdout, "You can also rotate camera with left mouse button dragging.\n");

  return true;
}

void SceneCameraTest::update()
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

  if (which_camera_ == 0) {
    qcamera_.update();
  } else {
    camera_.update(delta);
  }
}

void SceneCameraTest::render()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  prog_.use();

  if (imgui_) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Once);
    ImGui::Begin("config", &imgui_, IMGUI_SIMPLE_DIALOG_FLAGS);

    ImGui::RadioButton("QuatCamera", &which_camera_, 0); ImGui::SameLine();
    ImGui::RadioButton("EulerCamera", &which_camera_, 1);
    ImGui::End();
  }
  
  view_ = which_camera_ == 0 ? qcamera_.view_matrix() : camera_.view_matrix();
  proj_ = glm::perspective(glm::radians((which_camera_ == 0 ? qcamera_.zoom() : camera_.zoom())),
			   ScreenManager::aspectf(), 0.1f, 100.f);

  // world space positions of our cubes
  vec3 cube_positions[10] = {
			  vec3( 0.0f,  0.0f,  0.0f),
			  vec3( 2.0f,  5.0f, -15.0f),
			  vec3(-1.5f, -2.2f, -2.5f),
			  vec3(-3.8f, -2.0f, -12.3f),
			  vec3( 2.4f, -0.4f, -3.5f),
			  vec3(-1.7f,  3.0f, -7.5f),
			  vec3( 1.3f, -2.0f, -2.5f),
			  vec3( 1.5f,  2.0f, -2.5f),
			  vec3( 1.5f,  0.2f, -1.5f),
			  vec3(-1.3f,  1.0f, -1.5f)
  };

  for (size_t i = 0; i < 10; ++i) {
    model_ = glm::translate(mat4(1.f), cube_positions[i]);
    model_ = glm::rotate(model_, glm::radians(20.f * i), glm::vec3(1.f, 0.3f, 0.5f));
    prog_.set_uniform("MVP", proj_ * view_ * model_);
    cube_.render();
  }
  
  check_gl_error(__FILE__, __LINE__);
}

bool SceneCameraTest::compile_and_link_shaders()
{
  if (!prog_.compile_shader_from_file("shader/cameratest.vs", ShaderType::VERTEX)) {
    fprintf(stderr, "Compiling vertex shader failed.\n%s\n", prog_.log().c_str());
    return false;
  }
  if (!prog_.compile_shader_from_file("shader/cameratest.fs", ShaderType::FRAGMENT)) {
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
