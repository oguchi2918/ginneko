#version 410 core

out vec4 FragColor;

uniform vec4 color_table[] =
  { vec4(0.8f, 0.8f, 0.8f, 1.f),
    vec4(0.8f, 0.8f, 0.8f, 1.f),
    vec4(1.f, 0.f, 0.f, 1.f), };

void main()
{
  FragColor = color_table[gl_PrimitiveID];
}
