#ifndef INCLUDED_CAMERA_HPP
#define INCLUDED_CAMERA_HPP

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace nekolib {
  namespace camera {

    // simple camera class using euler angles (but not roll)
    class EulerCamera {
    public:
      enum class MoveDir { FORWARD = 0,
			   BACKWARD,
			   LEFT,
			   RIGHT };
      // ctor
      EulerCamera(glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f),
		  float yaw = -90.f, float pitch = 0.f) noexcept;
      // dtor
      ~EulerCamera() = default;

      // view matrix (lookAt)
      glm::mat4 view_matrix() const noexcept;

      void update(float) noexcept;
      
      glm::vec3 pos() const noexcept {
	return pos_;
      }

      float zoom() const noexcept {
	return zoom_;
      }

    private:
      // position and direction
      glm::vec3 pos_;
      glm::vec3 front_;
      glm::vec3 up_;
      glm::vec3 right_;
      glm::vec3 world_up_;

      // euler angles (degree)
      float yaw_;
      float pitch_;
      
      // zoom fovy
      float zoom_ = 45.f; // degree

      // drag start position
      int sx_ = 0;
      int sy_ = 0;

      void recalc_dirs(float, float) noexcept;

      // move camera
      void walk(MoveDir dir, float delta_time) noexcept;
      void move(glm::vec3 pos) noexcept {
	pos_ = pos;
      }
      // rotate camera
      void turn(float xoffset, float yoffset, bool update_flag = false) noexcept;
      // zoom camera
      void recalc_zoom(float yoffset) noexcept;

    public:
      // camera options
      float move_speed_ = 2.5f;
      float mouse_sensivity_ = 0.1f;

    };

    // Camera class using quaternion
    class QuatCamera {
    public:
      enum class DragMode { NONE = 0,
			    ROTATE = 1,
			    LEFT = 1,
			    BANK_OR_APROACH = 2,
			    RIGHT = 2,
			    RIGHT_OR_UP = 3,
			    BOTH = 3, };

      // ctor and dtor
      QuatCamera(glm::vec3 pos,
		 glm::vec3 target = glm::vec3(0.f, 0.f, 0.f),
		 glm::vec3 up = glm::vec3(0.f, 1.f, 0.f)) noexcept;
      ~QuatCamera() = default;

      void update() noexcept;
      
      // view matrix (lookAt)
      glm::mat4 view_matrix() const noexcept;

      float zoom() const noexcept { return zoom_; }

    private:
      // position, rotation, distance
      glm::vec3 target_; // camera target position
      glm::quat cq_; // camera rotation
      float distance_; // distance between camera and target

      // delta
      glm::vec3 dtarget_ = glm::vec3(0.f, 0.f ,0.f); // camera target position delta
      glm::quat dq_ = glm::quat(1.f, 0.f, 0.f, 0.f); // camera rotation delta
      float ddistance_ = 0.f;

      const glm::vec3 org_front_ = glm::vec3(0.f, 0.f, -1.f);
      const glm::vec3 org_up_ = glm::vec3(0.f, 1.f, 0.f);

      glm::vec3 front_;
      glm::vec3 up_;

      // zoom fovy
      float zoom_ = 45.f; // degree

      // drag data
      DragMode dmode_ = DragMode::NONE; // drag mode
      int drag_sx_ = 0; // drag start mouse x
      int drag_sy_ = 0; // drag start mouse y

      void reset_delta() noexcept;
      void update_dirs() noexcept;
      void move_front(int dy) noexcept;
      void process_drag(DragMode mode, int dx, int dy) noexcept;
    public:      
      float move_speed_ = 0.005f;
    };
  }
}

#endif // INCLUDED_CAMERA_HPP
