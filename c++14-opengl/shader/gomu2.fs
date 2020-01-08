#version 330 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out uint PickID;

in block {
  flat float fix_flag;
  flat int pick_id;
} fs_in;

uniform vec4 color_move;
uniform vec4 color_fix;

void main()
{
  FragColor = step(0.5, fs_in.fix_flag) * color_move + step(fs_in.fix_flag, 0.5) * color_fix;
  PickID = uint(fs_in.pick_id);
}
