#version 430 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// 節点
struct Point
{
  vec4 position;
};

// 節点群(t)
layout(std430, binding = 1) buffer ReadPoints
{
  readonly Point current_points[];
};

// 節点群(t-dt)
layout(std430, binding = 0) buffer WritePoints
{
  writeonly Point prev_points[];
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
  // 初速は0、両端以外にかかる加速度はgのみという条件で
  // 現在位置から一つ前の位置を逆算する
  
  // pos(-dt) = pos(0) - v(0)dt + a(0)dt^2 / 2
  prev_points[0].position = current_points[0].position;
  prev_points[point_num - 1].position = current_points[point_num - 1].position;

  for (uint i = 1; i < point_num - 1; ++i) {
    prev_points[i].position = current_points[i].position + 0.5 * dt * dt * vec4(g, 0.f, 0.f);
  }
}
