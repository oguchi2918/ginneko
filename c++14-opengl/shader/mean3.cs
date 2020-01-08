#version 430 core
layout (local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba8) uniform image2D SourceImage;
layout(binding = 1, rgba8) uniform image2D ResultImage;

void main()
{
  uint u = gl_GlobalInvocationID.x;
  uint v = gl_GlobalInvocationID.y;

  vec4 mean3 = vec4(0.f, 0.f, 0.f, 0.f);
  for (int du = -1; du <= 1; ++du) {
    for (int dv = -1; dv <= 1; ++dv) {
      mean3 += imageLoad(SourceImage, ivec2(u + du, v + dv));
    }
  }
  
  mean3 *= 0.11111111;

  imageStore(ResultImage, ivec2(u, v), vec4(mean3.rgb, 1.f));
}
