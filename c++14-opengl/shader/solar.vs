#version 410 core

layout (location = 0) in dvec3 aPos;
layout (location = 1) in double aMass;

out VS_OUT {
  flat float mass; // GL_POINTSではflat必須
} vs_out;

void main()
{
  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.f);
  gl_PointSize = floor((log(float(aMass)) / log(10) + 1.f) * 2);
  vs_out.mass = float(aMass);
}
