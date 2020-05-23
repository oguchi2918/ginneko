#version 410 core

in VS_OUT {
  flat float mass; //  GL_POINTSでもflat必須
} fs_in;

out vec4 FragColor;

uniform vec4 color_table[] =
  { vec4(1.f, 0.1f, 0.1f, 1.f),
    vec4(0.275f, 0.510f, 0.706f, 1.f),
    vec4(0.196, 0.804f, 0.196f, 1.f),
    vec4(0.933f, 0.902f, 0.522f, 1.f),
    vec4(0.410f, 0.416f, 0.416f, 1.f),
    vec4(0.521f, 0.032f, 0.521f, 1.f),
  };

void main()
{
  FragColor = color_table[int(fs_in.mass) % 1000];
}
