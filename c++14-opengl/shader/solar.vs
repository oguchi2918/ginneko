#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in float aMass;

out VS_OUT {
  float mass;
} vs_out;

void main()
{
  gl_Position = vec4(vec3(aPos), 1.f);
  gl_PointSize = floor((log(aMass) / log(10) + 1.f) * 2);
  vs_out.mass = aMass;
}
