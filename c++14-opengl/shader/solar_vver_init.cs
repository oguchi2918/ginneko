#version 430 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1)in;

// 質点
struct Point
{
  float mass; // 質量
  vec3 position; // 位置 p(h)
  vec3 velocity; // 速度 v(h)
  vec3 position_temp; // p(t+h)
  vec3 velocity_temp; // v(t+h)の途中経過
};

layout (std140) uniform PhysicParams
{
  uint point_num; // 質点数
  float dt; // タイムステップ
  float g; // 重力加速度
  float r_threshold; // 引力が発生する距離の閾値
};

// 質点群(更新前)
layout(std430, binding = 0) buffer ReadPoints
{
  readonly Point current_points[];
};

// 質点群(更新後)
layout(std430, binding = 1) buffer WritePoints
{
  writeonly Point next_points[];
};


// p(t)からa(t)を計算
vec3 calc_accel(uint i)
{
  vec2 a = vec2(0.0);

  // 自分以外の全質点に対して相互距離が一定以上なら
  // 相互距離の一乗に反比例する万有引力(/自分の質量)を計算して加算
  for (uint j = 0; j < i; ++j) {
    vec2 dpos = current_points[j].position.xy - current_points[i].position.xy;
    float r = length(dpos);
    a += step(r_threshold, r) * normalize(dpos) * g * current_points[j].mass / r;
  }
  for (uint j = i + 1; j < point_num; ++j) {
    vec2 dpos = current_points[j].position.xy - current_points[i].position.xy;
    float r = length(dpos);
    a += step(r_threshold, r) * normalize(dpos) * g * current_points[j].mass / r;
  }

  return vec3(a, 0.0);
}

void main()
{
  // 計算対象のindex
  const uint i = gl_WorkGroupID.x;

  // p(t), v(t)
  next_points[i].position = current_points[i].position;
  next_points[i].velocity = current_points[i].velocity;

  // 時刻tでの加速度
  vec3 a = calc_accel(i);

  // 位置 p(t + h)
  vec3 delta = dt * current_points[i].velocity + 0.5 * dt * dt * a;
  next_points[i].position_temp = current_points[i].position + delta;

  // 速度 v(t + h)は未完成
  delta = 0.5 * dt * a;
  next_points[i].velocity_temp = current_points[i].velocity + delta;
}
