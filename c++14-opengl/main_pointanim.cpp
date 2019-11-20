#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <glad/glad.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

#include "renderer.hpp"
#include "memory.hpp"
#include "input.hpp"
#include "clock.hpp"
#include "scene_pointanim.hpp"

const char* TITLE = "pointanim";

static SDL_Window* window = nullptr;
static SDL_GLContext context = nullptr;
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

static ScenePointAnim* scene = nullptr;

bool update()
{
  nekolib::memory::Manager::update();
  nekolib::clock::Manager::update();
  if (!nekolib::input::Manager::update()) {
    return false;
  }

  // TODO:
  scene->update();

  return true;
}

void draw()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(window);
  ImGui::NewFrame();

  // TODO:
  scene->render();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  SDL_GL_SwapWindow(window);
}

bool init(void)
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    fprintf(stderr, "SDL初期化に失敗:%s\n", SDL_GetError());
    return false;
  }

  // OpenGL 4.3 Core profile
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  
  // Debug output
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
  window = SDL_CreateWindow(TITLE,
			    SDL_WINDOWPOS_CENTERED,
			    SDL_WINDOWPOS_CENTERED,
			    SCREEN_WIDTH, SCREEN_HEIGHT,
			    SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

  if (window) {
    context = SDL_GL_CreateContext(window);
  }
  if (!window || !context) {
    fprintf(stderr, "画面初期化に失敗:%s\n", SDL_GetError());
    SDL_Quit();
    return false;
  }
  
  gladLoadGLLoader(SDL_GL_GetProcAddress);
  fprintf(stderr, "Vendor: %s\n", glGetString(GL_VENDOR));
  fprintf(stderr, "Renderer: %s\n", glGetString(GL_RENDERER));
  fprintf(stderr, "Version: %s\n", glGetString(GL_VERSION));

  // vsync
  if (SDL_GL_SetSwapInterval(1) < 0) {
    fprintf(stderr, "Warning: Unable to set Vsync! SDL Error:%s\n", SDL_GetError());
  }

  nekolib::renderer::regist_debug_callback();

  nekolib::memory::Manager::init();
  nekolib::clock::Manager::init();
  nekolib::input::Manager::init();
  nekolib::renderer::ScreenManager::init(SCREEN_WIDTH, SCREEN_HEIGHT);

  // imgui setup
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;

  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window, context);
  ImGui_ImplOpenGL3_Init("#version 410");

  // TODO:
  scene = new ScenePointAnim();
  if (!scene || !scene->init()) {
    return false;
  }
  return true;
}

void finalize()
{
  // TODO:
  delete scene;
  
  // imgui finalize
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  // SDL finalize
  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);

  SDL_Quit();
}

void main_loop()
{
  for (;;) {
    if (!update()) {
      break;
    }
    draw();
    SDL_Delay(0);
  }
}

int main(int argc, char* argv[])
{
  if (!init()) {
    return -1;
  }

  main_loop();
  finalize();

  return 0;
}
