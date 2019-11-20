#version 330 core

layout (location = 0) in vec4 aPos;

out VS_OUT {
  float fix_flag;
} vs_out;

void main()
{
  vs_out.fix_flag = aPos.w;
  gl_Position = vec4(vec3(aPos), 1.f);
}
