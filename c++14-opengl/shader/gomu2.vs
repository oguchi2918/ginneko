#version 330 core

layout (location = 0) in vec4 aPos; // 節点位置(xyz)+固定flag(w)

out block {
  flat float fix_flag;
  flat int pick_id;
} vs_out;

void main()
{
  vs_out.fix_flag = aPos.w;
  vs_out.pick_id = gl_VertexID;
  gl_Position = vec4(vec3(aPos), 1.f);
}
