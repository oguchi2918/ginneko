#include <cstdio>
#include <vector>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"

#include "scene_imageprocess.hpp"
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

bool SceneImageProcess::init(int* width, int* height)
{
  if (!compile_and_link_shaders()) {
    return false;
  }

  quad_ = Quad::create(0.f, 0.f, 1.f, 1.f);
  source_tex_ = Texture::create("texture/ginn_aomuke.png", true);
  if (!source_tex_) {
    fprintf(stderr, "can't open PNG file\n");
    return false;
  }
  *width = source_tex_.width();
  *height = source_tex_.height();
  
  result_tex_ = Texture::create("texture/ginn_aomuke.png", true);

  source_tex_.bind(1);
  result_tex_.bind(2);

  fprintf(stdout, "Press 'd' key to select filter dialog.\n");

  return true;
}

void SceneImageProcess::update()
{
  using namespace nekolib::input;
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
}

#define WORKGROUP_SIZE 16
void SceneImageProcess::filter(nekolib::renderer::Program& prog)
{
  prog.use();

  // OpenGL ESだとglBindImageTextureの対象はimmutableなTextureでなければならない
  // Desktop OpenGL + nvidia環境だと非immutableでも普通に動く模様
  // 公式APIリファレンスも明言していないのでimmutableなブツを使用

  // 一枚絵をそのままのサイズで表示するだけなのでmipmap level=0のみbind
  glBindImageTexture(0, source_tex_.handle(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
  glBindImageTexture(1, result_tex_.handle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

  glDispatchCompute(result_tex_.width() / WORKGROUP_SIZE + 1, result_tex_.height() / WORKGROUP_SIZE + 1, 1);

  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void SceneImageProcess::render()
{
  glClearColor(0.2f, 0.f, 0.2f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  
  static int e = 0;
  if (imgui_) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Once);
    ImGui::Begin("filters", &imgui_, IMGUI_SIMPLE_DIALOG_FLAGS);

    ImGui::RadioButton("None", &e, 0);
    ImGui::RadioButton("Invert", &e, 1);
    ImGui::RadioButton("Mean3x3", &e, 2);
    ImGui::RadioButton("Laplacian", &e, 3);
    ImGui::RadioButton("Sobel", &e, 4);

    ImGui::End();
  }

  switch (e) {
  case 0:
    source_tex_.bind(2);
    break;
  case 1:
    filter(invert_prog_);
    result_tex_.bind(2);
    break;
  case 2:
    filter(mean_prog_);
    result_tex_.bind(2);
    break;
  case 3:
    filter(laplacian_prog_);
    result_tex_.bind(2);
    break;
  case 4:
    filter(sobel_prog_);
    result_tex_.bind(2);
    break;
  default:
    assert(!"This must not be happen!");
    break;
  }
  
  prog_.use();
  prog_.set_uniform("Tex", 2);
  quad_.render();
}

bool SceneImageProcess::compile_and_link_shaders()
{
  if (!prog_.compile_shader_from_file("shader/quad.vs", ShaderType::VERTEX)) {
    fprintf(stderr, "Compiling vertex shader failed.\n%s\n", prog_.log().c_str());
    return false;
  }
  if (!prog_.compile_shader_from_file("shader/quad.fs", ShaderType::FRAGMENT)) {
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

  if (!invert_prog_.compile_shader_from_file("shader/invert.cs", ShaderType::COMPUTE)) {
    fprintf(stderr, "Compiling compute shader failed.\n%s\n", invert_prog_.log().c_str());
    return false;
  }
  if (!invert_prog_.link()) {
    fprintf(stderr, "Linking shader program failed.\n%s\n", invert_prog_.log().c_str());
    return false;
  }
  if (!invert_prog_.valid()) {
    fprintf(stderr, "Validating program failed.\n%s\n", invert_prog_.log().c_str());
    return false;
  }

  if (!mean_prog_.compile_shader_from_file("shader/mean3.cs", ShaderType::COMPUTE)) {
    fprintf(stderr, "Compiling compute shader failed.\n%s\n", mean_prog_.log().c_str());
    return false;
  }
  if (!mean_prog_.link()) {
    fprintf(stderr, "Linking shader program failed.\n%s\n", mean_prog_.log().c_str());
    return false;
  }
  if (!mean_prog_.valid()) {
    fprintf(stderr, "Validating program failed.\n%s\n", mean_prog_.log().c_str());
    return false;
  }

  if (!laplacian_prog_.compile_shader_from_file("shader/laplacian.cs", ShaderType::COMPUTE)) {
    fprintf(stderr, "Compiling compute shader failed.\n%s\n", laplacian_prog_.log().c_str());
    return false;
  }
  if (!laplacian_prog_.link()) {
    fprintf(stderr, "Linking shader program failed.\n%s\n", laplacian_prog_.log().c_str());
    return false;
  }
  if (!laplacian_prog_.valid()) {
    fprintf(stderr, "Validating program failed.\n%s\n", laplacian_prog_.log().c_str());
    return false;
  }
  
  if (!sobel_prog_.compile_shader_from_file("shader/sobel.cs", ShaderType::COMPUTE)) {
    fprintf(stderr, "Compiling compute shader failed.\n%s\n", sobel_prog_.log().c_str());
    return false;
  }
  if (!sobel_prog_.link()) {
    fprintf(stderr, "Linking shader program failed.\n%s\n", sobel_prog_.log().c_str());
    return false;
  }
  if (!sobel_prog_.valid()) {
    fprintf(stderr, "Validating program failed.\n%s\n", sobel_prog_.log().c_str());
    return false;
  }
  
  prog_.use();
  prog_.print_active_attribs();
  prog_.print_active_uniforms();

  return true;
}
