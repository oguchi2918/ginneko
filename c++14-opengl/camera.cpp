#include <cstdio>
#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "camera.hpp"
#include "input.hpp"
#include "renderer.hpp"

using glm::vec3;
using glm::vec4;
using glm::mat4;

using namespace nekolib::camera;

namespace nekolib {
  namespace camera {
    EulerCamera::EulerCamera(vec3 pos, float yaw, float pitch) noexcept
      : pos_(pos), world_up_(vec3(0.f, 1.f, 0.f)),
	yaw_(yaw), pitch_(pitch), zoom_(45.f),
	move_speed_(2.5f), mouse_sensivity_(0.1f)
    {
      recalc_dirs(yaw, pitch);
    }

    glm::mat4 EulerCamera::view_matrix() const noexcept
    {
      //	fprintf(stderr, "pos_: %f, %f, %f\n", pos_.x, pos_.y, pos_.z);
      //	fprintf(stderr, "target_: %f, %f, %f\n", (pos_ + front_).x, (pos_ + front_).y, (pos_ + front_).z);
      return glm::lookAt(pos_, pos_ + front_, up_);
    }

    void EulerCamera::update(float dt) noexcept
    {
      using namespace nekolib::input;
      Mouse m = nekolib::input::Manager::instance().mouse();
      Keyboard kb = nekolib::input::Manager::instance().keyboard();

      static bool dragging = false;

      // カメラ回転
      if (m.triggered(Mouse::Button::LEFT)) { // ドラッグ開始
	sx_ = m.x();
	sy_ = m.y();
	dragging = true;
      } else if (m.pushed(Mouse::Button::LEFT)) { // ドラッグ中
	turn(m.x() - sx_, -(m.y() - sy_));
      } else if (dragging) { // ドラッグ終了
	turn(m.x() - sx_, -(m.y() - sy_), true);
	dragging = false;
      }
      
      if (m.wheel_y()) {
	recalc_zoom(m.wheel_y());
      }

      if (kb.pushed(SDLK_LEFT)) {
	walk(MoveDir::LEFT, dt);
      }
      if (kb.pushed(SDLK_RIGHT)) {
	walk(MoveDir::RIGHT, dt);
      }
      if (kb.pushed(SDLK_UP)) {
	walk(MoveDir::FORWARD, dt);
      }
      if (kb.pushed(SDLK_DOWN)) {
	walk(MoveDir::BACKWARD, dt);
      }
    }
    
    void EulerCamera::walk(EulerCamera::MoveDir dir, float delta_time) noexcept
    {
      float velocity = move_speed_ * delta_time;
      if (dir == MoveDir::FORWARD) {
	//	  fprintf(stderr, "%lf\n", glm::length(front_) * velocity);
	pos_ += front_ * velocity;
      } else if (dir == MoveDir::BACKWARD) {
	pos_ -= front_ * velocity;
      } else if (dir == MoveDir::LEFT) {
	pos_ -= right_ * velocity;
      } else if (dir == MoveDir::RIGHT) {
	pos_ += right_ * velocity;
      } else {
	assert(!"This must not be happend."); 
      }
    }

    void EulerCamera::turn(float xoffset, float yoffset, bool update_flag) noexcept
    {
      xoffset *= mouse_sensivity_;
      yoffset *= mouse_sensivity_;

      float yaw = yaw_ + xoffset;
      float pitch = pitch_ + yoffset;

      if (pitch > 89.f) {
	pitch = 89.f;
      }
      if (pitch < -89.f) {
	pitch = -89.f;
      }

      recalc_dirs(yaw, pitch);
      if (update_flag) {
	yaw_ = yaw;
	pitch_ = pitch;
      }
    }

    void EulerCamera::recalc_zoom(float yoffset) noexcept
    {
      zoom_ -= yoffset;
      if (zoom_ <= 1.f) {
	zoom_ = 1.f;
      } else if (zoom_ >= 45.f) {
	zoom_ = 45.f;
      }
    }

    void EulerCamera::recalc_dirs(float yaw, float pitch) noexcept
    {
      // recalc front vector from auler angles
      glm::vec3 front;
      front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
      front.y = sin(glm::radians(pitch));
      front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
      front_ = glm::normalize(front);

      // recalc right and up vector
      right_ = glm::normalize(glm::cross(front_, world_up_));
      up_ = glm::normalize(glm::cross(right_, front_));
    }

    QuatCamera::QuatCamera(vec3 pos, vec3 target, vec3 up) noexcept
      : target_(target)
    {
      front_ = glm::normalize(target_ - pos);
      distance_ = glm::length(target_ - pos);
      glm::vec3 right = glm::normalize(glm::cross(front_, up));
      up_ = glm::normalize(glm::cross(right, front_));
	
      cq_ = glm::rotation(org_front_, front_);
      glm::vec3 up2 = cq_ * org_up_;
      cq_ = glm::rotation(up2, up_) * cq_;
    }

    void QuatCamera::reset_delta() noexcept
    {
      target_ += dtarget_;
      cq_ = cq_ * dq_;
      distance_ += ddistance_;
      dtarget_ = vec3(0.f, 0.f, 0.f);
      dq_ = glm::quat(1.f, 0.f, 0.f, 0.f);
      ddistance_ = 0.f;
    }

    void QuatCamera::update_dirs() noexcept
    {
      front_ = glm::normalize(cq_ * dq_ * org_front_);
      up_ = glm::normalize(cq_ * dq_ * org_up_);
    }

    void QuatCamera::update() noexcept
    {
      using namespace nekolib::input;
      Mouse m = nekolib::input::Manager::instance().mouse();
      
      bool l = m.pushed(Mouse::Button::LEFT), r = m.pushed(Mouse::Button::RIGHT);
      int x = m.x(), y = m.y();
      
      DragMode dmode;
      if (l && !r) { // left only
	dmode = DragMode::LEFT;
      } else if (!l && r) { // right only
	dmode = DragMode::RIGHT;
      } else if (l && r) { // both
	dmode = DragMode::BOTH;
      } else { // none
	dmode = DragMode::NONE;
      }

      if (dmode == dmode_) { // drag継続中
	process_drag(dmode_, x - drag_sx_, y - drag_sy_);
      } else { // drag終了
	process_drag(dmode_, x - drag_sx_, y - drag_sy_);
	reset_delta();
	dmode_ = dmode;
	drag_sx_ = x; drag_sy_ = y;
      }

      if (m.wheel_y()) {
	move_front(m.wheel_y());
      }
    }

    void QuatCamera::move_front(int dy) noexcept
    {
      // 宅のX環境だと毎フレーム1or2しかdyが来ないので10倍している
      // wheel event1回につき+1しかされない模様
      target_ -= move_speed_ * dy * 10.f * front_;
    }

    void QuatCamera::process_drag(DragMode mode, int dx, int dy) noexcept
    {
      if (dx == 0 && dy == 0) {
	return;
      }
      //fprintf(stderr, "update: %d, %d, %f\n", dx, dy, delta_time);
      if (mode == DragMode::NONE) {
	// nothing to do
      } else if (mode == DragMode::ROTATE) {
	// rotate camera position around target
	float fx = static_cast<float>(-dx) / nekolib::renderer::ScreenManager::width();
	float fy = static_cast<float>(-dy) / nekolib::renderer::ScreenManager::height();

	float ar = sqrt(fx * fx + fy * fy) * glm::two_pi<float>();
	dq_ = glm::rotate(glm::quat(1.f, 0.f, 0.f, 0.f), ar, glm::vec3(fy, fx, 0.f));
	update_dirs();
      } else if (mode == DragMode::BANK_OR_APROACH) {
	float len = move_speed_ * dy;
	if (distance_ >= len + 0.05f) {
	  ddistance_ = -len;
	} else {
	  ddistance_ = -(distance_ - 0.05f);
	}

	float ar = glm::two_pi<float>() * dx / nekolib::renderer::ScreenManager::width();
	dq_ = glm::rotate(glm::quat(1.f, 0.f, 0.f, 0.f), ar, org_front_);
	update_dirs();
      } else if (mode == DragMode::RIGHT_OR_UP) {
	vec3 right = glm::normalize(glm::cross(front_, up_));
	vec3 d = static_cast<float>(dx) * right + static_cast<float>(-dy) * up_;
	dtarget_ = move_speed_ * d;
      }
    }

    glm::mat4 QuatCamera::view_matrix() const noexcept
    {
      glm::vec3 target = target_ + dtarget_;
      float distance = distance_ + ddistance_;
      
      return glm::lookAt(target - (distance * front_), target, up_);
    }
  }
}
