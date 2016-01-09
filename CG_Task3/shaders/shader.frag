#version 330

in vec2 TexCoords;
in vec4 ProjCoords;

out vec4 color;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

uniform sampler2D projectiveTexture;

void main()
{
	vec4 diffuse_color = texture(texture_diffuse1, TexCoords);
	vec2 ndc = (ProjCoords.xy / ProjCoords.w) / 2.0 + 0.5;
	vec4 proj_color = vec4(0.0);
	if (ProjCoords.q > 0.0)
	{
		proj_color = texture(projectiveTexture, vec2(ndc.x, 1.0 - ndc.y));
	}
	color = diffuse_color + 0.4 * proj_color;
}