#version 330 core

layout (location = 0) in vec4 aPos; // 節点位置(xyz)+固定flag(w)

void main()
{
  gl_Position = vec4(vec3(aPos), 1.f);
}
