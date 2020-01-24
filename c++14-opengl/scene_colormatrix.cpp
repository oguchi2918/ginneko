#include <cstdio>
#include <vector>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui/imgui.h"

#include "scene_colormatrix.hpp"
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

bool SceneColorMatrix::init(int* width, int* height)
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

  fprintf(stdout, "Press 'd' key to show/hide color dialog.\n");

  return true;
}

void SceneColorMatrix::update()
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
void SceneColorMatrix::compute()
{
  // OpenGL ESだとglBindImageTextureの対象はimmutableなTextureでなければならない
  // Desktop OpenGL + nvidia環境だと非immutableでも普通に動く模様
  // 公式APIリファレンスも明言していないのでimmutableなブツを使用

  // 一枚絵をそのままのサイズで表示するだけなのでmipmap level=0のみbind
  glBindImageTexture(0, source_tex_.handle(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
  glBindImageTexture(1, result_tex_.handle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

  glDispatchCompute(result_tex_.width() / WORKGROUP_SIZE + 1, result_tex_.height() / WORKGROUP_SIZE + 1, 1);

  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void SceneColorMatrix::render()
{
  glClearColor(0.2f, 0.f, 0.2f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  
  static float brightness = 0.f;
  static float contrast = 1.f;
  static float saturation = 1.f;
  static int hue = 0;
  static float mono_s = 0.f;
  static vec3 mono_color(0.4f, 0.3f, 0.15f); // セピア?
  
  if (imgui_) {
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Once);
    ImGui::Begin("color correction", &imgui_, IMGUI_SIMPLE_DIALOG_FLAGS);

    ImGui::SliderFloat("Brightness", &brightness, -1.f, 1.f);
    ImGui::SliderFloat("Contrast", &contrast, -1.f, 3.f);
    ImGui::SliderFloat("Saturation", &saturation, 0.f, 3.f);
    ImGui::SliderInt("Hue", &hue, 0, 360);
    ImGui::SliderFloat("Monotone filter strength", &mono_s, 0.f, 1.f);
    ImGui::ColorEdit3("Monotone filter color", &mono_color.r);

    ImGui::End();
  }

  // 各種Color Matrixを作成
  // 最初の3種類については
  // https://docs.rainmeter.net/tips/colormatrix-guide/ の解説を参照
  // ↑では5x5行列だが以下のコードではアルファは変換対象外として
  // 4x4行列によるアフィン変換を実装している
  mat4 bm(1.f); // brightness matrix
  mat4 cm(1.f); // contrast matrix
  mat4 sm(1.f); // saturation matrix
  mat4 lm(1.f); // hue matrix (rgbに対するrotate)
  mat4 mono(1.f); // monotone filter (単色と変化強度で乗算)

  const vec3 lumRGB(0.3086, 0.6094, 0.0820);
  bm[3] = vec4(brightness, brightness, brightness, 1.f);
  cm[0][0] = cm[1][1] = cm[2][2] = contrast;
  float tmp = (1.f - contrast) * 0.5f;
  cm[3] = vec4(tmp, tmp, tmp, 1.f);
  tmp = (1 - saturation) * lumRGB.r;
  sm[0] = vec4(tmp + saturation, tmp, tmp, 0.f);
  tmp = (1 - saturation) * lumRGB.g;
  sm[1] = vec4(tmp, tmp + saturation, tmp, 0.f);
  tmp = (1 - saturation) * lumRGB.b;
  sm[2] = vec4(tmp, tmp, tmp + saturation, 0.f);
  lm = glm::rotate(lm, glm::radians(static_cast<float>(hue)), vec3(1.f, 1.f, 1.f));
  mono[0] = vec4((1.f - mono_s) + mono_s * mono_color.r,
		 mono_s * mono_color.g,
		 mono_s * mono_color.b,
		 0.f);
  mono[1] = vec4(mono_s * mono_color.r,
		 (1.f - mono_s) + mono_s * mono_color.g,
		 mono_s * mono_color.b,
		 0.f);
  mono[2] = vec4(mono_s * mono_color.r,
		 mono_s * mono_color.g,
		 (1.f - mono_s) + mono_s * mono_color.b,
		 0.f);

  cm_prog_.use();
  //行列の掛算なので一部を除き交換則は成り立たない
  //今回はダイアログで上から表示される順に色変換を適用する仕様とする
  cm_prog_.set_uniform("ColorMatrix", mono * lm * sm * cm * bm);
  compute();
  
  prog_.use();
  prog_.set_uniform("Tex", 2);
  quad_.render();
}

bool SceneColorMatrix::compile_and_link_shaders()
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

  if (!cm_prog_.compile_shader_from_file("shader/colormatrix.cs", ShaderType::COMPUTE)) {
    fprintf(stderr, "Compiling compute shader failed.\n%s\n", cm_prog_.log().c_str());
    return false;
  }
  if (!cm_prog_.link()) {
    fprintf(stderr, "Linking shader program failed.\n%s\n", cm_prog_.log().c_str());
    return false;
  }
  if (!cm_prog_.valid()) {
    fprintf(stderr, "Validating program failed.\n%s\n", cm_prog_.log().c_str());
    return false;
  }

  prog_.use();
  prog_.print_active_attribs();
  prog_.print_active_uniforms();

  return true;
}
