#version 330 core

out vec4 FragColor;

in VS_OUT {
  float fix_flag;
} fs_in;

uniform vec4 color_move;
uniform vec4 color_fix;

void main()
{
  FragColor = step(0.5, fs_in.fix_flag) * color_move + step(fs_in.fix_flag, 0.5) * color_fix;
}
