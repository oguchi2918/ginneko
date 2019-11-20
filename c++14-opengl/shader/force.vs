#version 330

out vec4 position;

uniform samplerBuffer table;

const int left_id = 0;
const int right_id = 19;
const float l = 0.1;
const float k = 0.1;
const vec2 g = vec2(0.f, -0.001);

void main()
{
  vec4 p0 = texelFetch(table, gl_VertexID);

  if (p0.w != 0.f) {
    vec2 p1 = texelFetch(table, gl_VertexID - 1).xy;
    vec2 p2 = texelFetch(table, gl_VertexID + 1).xy;

    vec2 l1 = p1 - p0.xy;
    vec2 l2 = p2 - p0.xy;
    vec2 f1 = (length(l1) - l) * k * normalize(l1);
    vec2 f2 = (length(l2) - l) * k * normalize(l2);

    vec2 f = step(float(left_id + 1), gl_VertexID) * f1 + step(gl_VertexID, float(right_id - 1)) * f2 + g;
    p0 += vec4(f, 0.f, 0.f);
  }

  position = p0;
}
