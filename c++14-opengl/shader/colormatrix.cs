#version 430 core
layout (local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba8) uniform image2D SourceImage;
layout(binding = 1, rgba8) uniform image2D ResultImage;

uniform mat4 ColorMatrix;

void main()
{
  uint u = gl_GlobalInvocationID.x;
  uint v = gl_GlobalInvocationID.y;

  vec4 org_color = imageLoad(SourceImage, ivec2(u, v));
  float alpha = org_color.w; // alphaは変換対象から外す

  imageStore(ResultImage, ivec2(u, v), vec4((ColorMatrix * vec4(org_color.rgb, 1.f)).rgb, alpha));
}
