#version 430 core
layout (local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba8) uniform image2D SourceImage;
layout(binding = 1, rgba8) uniform image2D ResultImage;

void main()
{
  uint u = gl_GlobalInvocationID.x;
  uint v = gl_GlobalInvocationID.y;

  vec4 inv = 1.f - imageLoad(SourceImage, ivec2(u, v));
  imageStore(ResultImage, ivec2(u, v), inv);
}
