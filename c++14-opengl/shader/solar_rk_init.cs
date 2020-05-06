#version 430 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1)in;

// 質点
struct Point
{
  float mass; // 質量
  vec3 position; // 位置
  vec3 velocity; // 速度
  vec3 position_temp; // 位置(最終結果)
  vec3 velocity_temp; // 速度(最終結果)
};

// 質点群(元値)
layout(std430, binding = 0) buffer ReadWritePoints
{
  Point current_points[];
};

// 質点群(仮値)
layout(std430, binding = 1) buffer WritePoints
{
  writeonly Point next_points[];
};

layout (std140) uniform PhysicParams
{
  uint point_num; // 質点数
  float dt; // タイムステップ
  float g; // 重力加速度
  float r_threshold; // 引力が発生する距離の閾値
};

void main()
{
  // 計算対象のindex
  const uint i = gl_WorkGroupID.x;

  current_points[i].position_temp = current_points[i].position;
  current_points[i].velocity_temp = current_points[i].velocity;
  next_points[i].position_temp = current_points[i].position;
  next_points[i].velocity_temp = current_points[i].velocity;
}
