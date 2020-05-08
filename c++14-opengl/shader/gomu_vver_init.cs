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

// 節点群
layout(std430, binding = 0) buffer ReadWritePoints
{
  Point current_points[];
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
  // position_tempを正しい初期状態に設定する
  for (uint i = 0; i < point_num; ++i) {
    current_points[i].position_temp = current_points[i].position;
  }

  // velocity_tempを正しい初期状態に設定する
  // 両端は固定されており力が働いていない状態なので補正の必要なし
  current_points[0].velocity_temp = current_points[0].velocity;
  current_points[point_num - 1].velocity_temp = current_points[point_num - 1].velocity;
  
  for (uint i = 1; i < point_num - 1; ++i) {
    // このままだと初速に0.5 * dt * gが加算されてしまうので
    // あらかじめ引いた値をvelocity_tempとして設定
    current_points[i].velocity_temp = current_points[i].velocity - 0.5 * dt * vec3(g, 0.f);
  }
}
