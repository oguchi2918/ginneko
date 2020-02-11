#version 430 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// 節点
struct Point
{
  vec4 position;
};

// 節点群(t-dt)
layout(std430, binding = 0) buffer ReadPoints
{
  readonly Point prev_points[];
};

// 節点群(t)
layout(std430, binding = 1) buffer ReadPoints2
{
  readonly Point current_points[];
};

// 節点群(t+dt)
layout(std430, binding = 2) buffer WritePoints
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
  // 両端の節点のみ計算する

  // 左端
  // 速度
  vec2 v = (current_points[0].position.xy - prev_points[0].position.xy) / dt;
  vec2 v2 = (current_points[1].position.xy - prev_points[1].position.xy) / dt;
    
  // 力
  vec2 l2 = current_points[1].position.xy - current_points[0].position.xy;
  vec2 dv2 = v - v2;
  vec2 f2 = (length(l2) - l) * k * normalize(l2) - c * dv2;

  vec3 a = vec3(f2 / m + g, 0.f);

  // 位置
  next_points[0].position = current_points[0].position +
    step(0.5, current_points[0].position.w) *
    vec4(current_points[0].position.xyz - prev_points[0].position.xyz + a * dt * dt, 0.f);

  // 右端
  // 速度
  v = (current_points[point_num - 1].position.xy - prev_points[point_num - 1].position.xy) / dt;
  vec2 v1 = (current_points[point_num - 2].position.xy - prev_points[point_num - 2].position.xy) / dt;
    
  // 力
  vec2 l1 = current_points[point_num - 2].position.xy - current_points[point_num - 1].position.xy;
  vec2 dv1 = v - v1;
  vec2 f1 = (length(l1) - l) * k * normalize(l1) - c * dv1;

  a = vec3(f1 / m + g, 0.f);

  // 位置
  next_points[point_num - 1].position = current_points[point_num - 1].position +
    step(0.5, current_points[point_num - 1].position.w) *
    vec4(current_points[point_num - 1].position.xyz - prev_points[point_num - 1].position.xyz + a * dt * dt, 0.f);
}
