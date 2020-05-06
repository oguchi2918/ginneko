#version 330 core

uniform sampler2D Tex;
uniform float Factor = 0.f;

in vec2 TexCoord;

layout (location = 0) out vec4 Color;

void main()
{
  vec4 TexColor = texture(Tex, TexCoord);
  TexColor.a -= Factor;
  Color = TexColor;
}
