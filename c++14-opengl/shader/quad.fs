#version 330 core

uniform sampler2D Tex;

in vec2 TexCoord;

layout (location = 0) out vec4 Color;

void main()
{
  Color = texture(Tex, TexCoord);
}
