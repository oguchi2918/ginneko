#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in float aMass;

out VS_OUT {
  flat float mass; // GL_POINTSではflat必須
} vs_out;

uniform mat4 MVP;

void main()
{
  vs_out.mass = aMass;
  gl_PointSize = floor((log(aMass) / log(10) + 1.f) * 2);
  gl_Position = MVP * vec4(aPos, 1.f);
}
