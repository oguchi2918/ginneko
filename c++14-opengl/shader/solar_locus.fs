#version 330 core

uniform sampler2D Tex;

// α値を下げていき閾値以下なら0にする
// (薄くなるとどの点の軌跡か区別が難しい為)
uniform float factor = 0.f;
uniform float alpha_threshold = 0.95f;

in vec2 TexCoord;

layout (location = 0) out vec4 Color;

void main()
{
  vec4 TexColor = texture(Tex, TexCoord);
  TexColor.a = step(alpha_threshold, TexColor.a) * (TexColor.a - factor);
  Color = TexColor;
}
