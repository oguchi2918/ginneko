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

// 質点群(オリジナル)
layout(std430, binding = 0) buffer OrgPoints
{
  readonly Point org_points[];
};

// 質点群(仮値A)
layout(std430, binding = 1) buffer ReadPoints
{
  readonly Point current_points[];
};

// 質点群(仮値B)
layout(std430, binding = 2) buffer WritePoints
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

uniform float x_dt1; // 更新時にdtに掛ける係数(仮値計算用)
uniform float x_dt2; // 更新時にdtに掛ける係数(最終結果計算用)


vec3 calc_accel(uint i)
{
  vec2 a = vec2(0.f);

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

  return vec3(a, 0.f);
}

// 仮値Aから加速度(力)を計算してオリジナルを参照しながら仮値Bを計算
// ついでに最終結果の方も少しづつ更新

void main()
{
  // 計算対象のindex
  const uint i = gl_WorkGroupID.x;

  vec3 a = calc_accel(i);

  // 仮値の速度/位置を更新
  vec3 delta = x_dt1 * dt * a;
  next_points[i].velocity = org_points[i].velocity + delta;
  delta = x_dt1 * dt * current_points[i].velocity;
  next_points[i].position = org_points[i].position + delta;

  // 最終結果の速度/位置を更新
  delta = x_dt2 * dt * a;
  next_points[i].velocity_temp = current_points[i].velocity_temp + delta;
  delta = x_dt2 * dt * current_points[i].velocity;
  next_points[i].position_temp = current_points[i].position_temp + delta;
}