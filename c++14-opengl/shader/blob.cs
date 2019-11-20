#version 430 core
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// 粒子
struct Particle
{
  vec4 position;
  vec4 velocity;
};

// 粒子群
layout(std430, binding = 0) buffer Particles
{
  Particle particles[];
};

// 重力加速度
const vec4 gravity = vec4(0.f, -9.8, 0.f, 0.f);

// 地面の高さ
uniform float height = -0.95;

// 減衰率
uniform float attenuation = 0.7;

// タイムステップ
uniform float dt;

void main()
{
  // Workgorup ID をそのまま頂点データのインデックスとして使用
  const uint i = gl_WorkGroupID.x;

  // 位置を更新
  particles[i].position += particles[i].velocity * dt;

  if (particles[i].position.y < height) {
    // y方向の速度を反転して減らす
    particles[i].velocity.y = -attenuation * particles[i].velocity.y;

    // 高さは地面から跳ね返った位置へ
    particles[i].position.y = height + attenuation * (height - particles[i].position.y);
  }
  
  // 速度を更新
  particles[i].velocity += gravity * dt;
}
