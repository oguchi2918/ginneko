#version 430 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// 節点
struct Point
{
  vec4 position; // 位置
  vec3 velocity; // 速度
  vec4 position_temp; // p(t + h)
  vec3 velocity_temp; // v(t + h)の途中
};

// 節点群(更新前)
layout(std430, binding = 0) buffer ReadPoints
{
  readonly Point current_points[];
};

// 節点群(更新後)
layout(std430, binding = 1) buffer WritePoints
{
  writeonly Point next_points[];
};

layout (std140) uniform PhysicParams
{
  uint point_num; // 節点数
  float dt; // タイムステップ
  float m; // 節点の質量
  float k; // ばね定数
  float c; // ばね減衰係数
};

float l = 2.f / point_num; // 節点間の自然長
vec2 g = vec2(0.f, -9.8f / point_num); // 重力

void main()
{
  // 左端
  // 位置 p(t + h)
  next_points[0].position = current_points[0].position_temp;
  
  // 加速度 a(t + h) (v(t + h)の値が不正確だがverlet法では仕方がない)
  vec2 l2 = current_points[1].position_temp.xy - current_points[0].position_temp.xy;
  vec2 dv2 = current_points[0].velocity_temp.xy - current_points[1].velocity_temp.xy;
  vec2 f2 = (length(l2) - l) * k * normalize(l2) - c * dv2;

  vec3 a = vec3(f2 / m + g, 0.f);

  // 速度 v(t + h)
  vec3 delta = 0.5 * dt * a;
  vec3 temp = step(0.5, current_points[0].position.w) *
    (current_points[0].velocity_temp + delta);
  next_points[0].velocity = temp;
  
  // 位置 p(t + 2h)
  delta = dt * temp + 0.5 * dt * dt * a;
  next_points[0].position_temp = current_points[0].position_temp +
    step(0.5, current_points[0].position_temp.w) * vec4(delta, 0.f);

  // 速度 v(t + 2h)は未完成
  delta = dt * a;
  next_points[0].velocity_temp = step(0.5, current_points[0].position_temp.w) *
    (current_points[0].velocity_temp + delta);

  // 右端
  // 位置 p(t + h)
  next_points[point_num - 1].position = current_points[point_num - 1].position_temp;
  
  // 加速度 a(t + h) (v(t + h)の値が不正確だがverlet法では仕方がない)
  vec2 l1 = current_points[point_num - 2].position_temp.xy - current_points[point_num - 1].position_temp.xy;
  vec2 dv1 = current_points[point_num - 1].velocity_temp.xy - current_points[point_num - 2].velocity_temp.xy;
  vec2 f1 = (length(l1) - l) * k * normalize(l1) - c * dv1;

  a = vec3(f1 / m + g, 0.f);

  // 速度 v(t + h)
  delta = 0.5 * dt * a;
  temp = step(0.5, current_points[point_num - 1].position.w) *
    (current_points[point_num - 1].velocity_temp + delta);
  next_points[point_num - 1].velocity = temp;
  
  // 位置 p(t + 2h)
  delta = dt * temp + 0.5 * dt * dt * a;
  next_points[point_num - 1].position_temp = current_points[point_num - 1].position_temp +
    step(0.5, current_points[point_num - 1].position_temp.w) * vec4(delta, 0.f);
  
  // 速度 v(t + 2h)は未完成
  delta = dt * a;
  next_points[point_num - 1].velocity_temp =
    step(0.5, current_points[point_num - 1].position_temp.w) *
    (current_points[point_num - 1].velocity_temp + delta);

}
