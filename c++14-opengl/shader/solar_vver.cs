#version 430 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1)in;

// 質点
struct Point
{
  double mass; // 質量
  dvec3 position; // 位置 p(h)
  dvec3 velocity; // 速度 v(h)
  dvec3 position_temp; // p(t+h)
  dvec3 velocity_temp; // v(t+h)の途中経過
};

layout (std140) uniform PhysicParams
{
  uint point_num; // 質点数
  double dt; // タイムステップ
  double g; // 重力加速度
  double r_threshold; // 引力が発生する距離の閾値
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


// p(t+h)からa(t+h)を計算するので注意
dvec3 calc_accel(uint i)
{
  dvec2 a = dvec2(0.0);

  // 自分以外の全質点に対して相互距離が一定以上なら
  // 相互距離の一乗に反比例する万有引力(/自分の質量)を計算して加算
  for (uint j = 0; j < i; ++j) {
    dvec2 dpos = current_points[j].position_temp.xy - current_points[i].position_temp.xy;
    double r = length(dpos);
    a += step(r_threshold, r) * normalize(dpos) * g * current_points[j].mass / r;
  }
  for (uint j = i + 1; j < point_num; ++j) {
    dvec2 dpos = current_points[j].position_temp.xy - current_points[i].position_temp.xy;
    double r = length(dpos);
    a += step(r_threshold, r) * normalize(dpos) * g * current_points[j].mass / r;
  }

  return dvec3(a, 0.0);
}

void main()
{
  // 計算対象のindex
  const uint i = gl_WorkGroupID.x;

  // 位置 p(t + h)
  next_points[i].position = current_points[i].position_temp;
  
  // 時刻t+hでの加速度
  dvec3 a = calc_accel(i);

  // 速度 v(t + h)
  dvec3 delta = 0.5 * dt * a;
  dvec3 temp = current_points[i].velocity_temp + delta; // next_pointsはreadonly
  next_points[i].velocity = temp;

  // 位置 p(t + 2h)
  delta = dt * temp + 0.5 * dt * dt * a;
  next_points[i].position_temp = current_points[i].position_temp + delta;

  // 速度 v(t + 2h)は未完成
  delta = dt * a;
  next_points[i].velocity_temp = current_points[i].velocity_temp + delta;
}
