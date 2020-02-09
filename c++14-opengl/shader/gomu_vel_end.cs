#version 430 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// 節点
struct Point
{
  vec4 position;
  vec3 velocity;
  vec3 force;
};

// 節点群(更新前)
layout(std430, binding = 0) buffer ReadPoints
{
  readonly Point read_points[];
};

// 節点群(更新後)
layout(std430, binding = 1) buffer WritePoints
{
  writeonly Point write_points[];
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
  // write_points[0]とwrite_point[point_num - 1]だけ計算する

  // write_points[0]
  vec2 p2 = read_points[1].position.xy;
  vec2 l2 = p2 - read_points[0].position.xy;
  vec2 dv2 = read_points[0].velocity.xy - read_points[1].velocity.xy;
  vec2 f2 = (length(l2) - l) * k * normalize(l2) - c * dv2;

  vec3 force = vec3(f2 + m * g, 0.f);
  vec3 delta_vel = dt * force / (2.f * m);
  
  write_points[0].position = read_points[0].position;
  write_points[0].velocity = step(0.5, read_points[0].position.w) * (read_points[0].velocity + delta_vel);
  write_points[0].force = force;

  // write_points[point_num - 1]
  vec2 p1 = read_points[point_num - 2].position.xy;
  vec2 l1 = p1 - read_points[point_num - 1].position.xy;
  vec2 dv1 = read_points[point_num - 1].velocity.xy - read_points[point_num - 2].velocity.xy;
  vec2 f1 = (length(l1) - l) * k * normalize(l1) - c * dv1;

  force = vec3(f1 + m * g, 0.f);
  delta_vel = dt * force / (2.f * m);

  write_points[point_num - 1].position = read_points[point_num - 1].position;
  write_points[point_num - 1].velocity = step(0.5, read_points[point_num - 1].position.w) *
    (read_points[point_num - 1].velocity + delta_vel);
  write_points[point_num - 1].force = force;
}
