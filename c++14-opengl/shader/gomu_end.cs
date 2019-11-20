#version 430 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// 節点
struct Point
{
  vec4 position;
  vec4 velocity;
};

uniform uint point_num;

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

const float m = 1.f; // 節点の質量
float l = 2.f / point_num; // 節点間の自然長
const float k = 250.f; // ばね定数
const float c = 30.f; // ばね減衰係数
vec2 g = vec2(0.f, -9.8f / point_num); // 重力

// タイムステップ
const float dt = 1.f / 60;

void main()
{
  // points[0]とpoint[point_num - 1]だけ計算する

  // points[0]
  vec2 p2 = read_points[1].position.xy;
  vec2 l2 = p2 - read_points[0].position.xy;
  vec2 dv2 = read_points[0].velocity.xy - read_points[1].velocity.xy;
  vec2 f2 = (length(l2) - l) * k * normalize(l2) - c * dv2;
  vec4 a = vec4(f2 / m + g, 0.f, 0.f);
  vec4 v_avg = read_points[0].velocity + (a * dt) * 0.5; // 修正オイラー法
    
  write_points[0].position = read_points[0].position + step(0.5, read_points[0].position.w) * v_avg * dt;
  write_points[0].velocity = read_points[0].velocity + step(0.5, read_points[0].position.w) * a * dt;

  // points[point_num - 1]
  vec2 p1 = read_points[point_num - 2].position.xy;
  vec2 l1 = p1 - read_points[point_num - 1].position.xy;
  vec2 dv1 = read_points[point_num - 1].velocity.xy - read_points[point_num - 2].velocity.xy;
  vec2 f1 = (length(l1) - l) * k * normalize(l1) - c * dv1;
    
  a = vec4(f1 / m + g, 0.f, 0.f);
  v_avg = read_points[point_num - 1].velocity + (a * dt) * 0.5; // 修正オイラー法
    
  write_points[point_num - 1].position = read_points[point_num - 1].position +
    step(0.5, read_points[point_num - 1].position.w) * v_avg * dt;
  write_points[point_num - 1].velocity = read_points[point_num - 1].velocity +
    step(0.5, read_points[point_num - 1].position.w) * a * dt;
}
