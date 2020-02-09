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
  // 力の計算をしないので端点を特別扱いする必要は無い
  // 頂点データ0番から
  const uint i = gl_WorkGroupID.x;

  // velocity verlet法による位置と速度の更新(前半)
  vec3 delta_pos = dt * read_points[i].velocity + dt * dt * read_points[i].force / (2.f * m);
  vec3 delta_vel = dt * read_points[i].force / (2.f * m);
  
  write_points[i].position = read_points[i].position + step(0.5, read_points[i].position.w) * vec4(delta_pos, 0.f);
  write_points[i].velocity = step(0.5, read_points[i].position.w) * (read_points[i].velocity + delta_vel);
  write_points[i].force = read_points[i].force;
}
