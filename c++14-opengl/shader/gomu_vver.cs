#version 430 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// 節点
struct Point
{
  vec4 position; // 位置
  vec3 velocity; // 速度
  vec3 velocity_temp; // 速度もどき
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
  // 頂点データ1番から
  const uint i = gl_WorkGroupID.x + 1;

  // 加速度(力)
  vec2 l1 = current_points[i - 1].position.xy - current_points[i].position.xy;
  vec2 dv1 = current_points[i].velocity.xy - current_points[i - 1].velocity.xy;
  vec2 f1 = (length(l1) - l) * k * normalize(l1) - c * dv1;

  vec2 l2 = current_points[i + 1].position.xy - current_points[i].position.xy;
  vec2 dv2 = current_points[i].velocity.xy - current_points[i + 1].velocity.xy;
  vec2 f2 = (length(l2) - l) * k * normalize(l2) - c * dv2;

  vec3 a = vec3((f1 + f2) / m + g, 0.f);

  // 速度
  vec3 delta_vel = 0.5 * dt * a;
  next_points[i].velocity = step(0.5, current_points[i].position.w) *
    (current_points[i].velocity_temp + delta_vel);
  
  // 速度もどき
  delta_vel = dt * a;
  next_points[i].velocity_temp = step(0.5, current_points[i].position.w) *
    (current_points[i].velocity_temp + delta_vel);

  // 位置
  vec3 delta_pos = dt * current_points[i].velocity + 0.5 * dt * dt * a;
  next_points[i].position = current_points[i].position +
    step(0.5, current_points[i].position.w) * vec4(delta_pos, 0.f);
}
