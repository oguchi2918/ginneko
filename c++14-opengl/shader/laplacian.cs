#version 430 core
layout (local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba8) uniform image2D SourceImage;
layout(binding = 1, rgba8) uniform image2D ResultImage;

void main()
{
  uint u = gl_GlobalInvocationID.x;
  uint v = gl_GlobalInvocationID.y;


  vec4 fc = imageLoad(SourceImage, ivec2(u, v)) * 4.0 -
    imageLoad(SourceImage, ivec2(u + 1, v)) -
    imageLoad(SourceImage, ivec2(u - 1, v)) -
    imageLoad(SourceImage, ivec2(u, v + 1)) -
    imageLoad(SourceImage, ivec2(u, v - 1));

  imageStore(ResultImage, ivec2(u, v), vec4(fc.rgb, 1.f));
}
