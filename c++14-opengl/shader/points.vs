#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 MVP;
uniform float elapsed_time;

void main()
{
  float z = fract(aPos.z - elapsed_time);
  gl_Position = MVP * vec4(aPos.x * z * z, aPos.y * z * z, z, 1.f);
}
