#version 330 core
in vec2 TexCoord;

out vec4 FragColor;

// texure samplers
uniform sampler2D ContainerTex;
uniform sampler2D FaceTex;

void main()
{
	// linearly interpolate (80% container, 20% face)
	FragColor = mix(texture(ContainerTex, TexCoord), texture(FaceTex, TexCoord), 0.2);
}
