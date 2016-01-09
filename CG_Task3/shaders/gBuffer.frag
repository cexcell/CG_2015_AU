#version 330

layout (location = 0) out vec4 gAlbedoSpec;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main()
{
	vec4 diffuse_color = texture(texture_diffuse1, TexCoords);
	gAlbedoSpec.rgba = diffuse_color.rgba;
}