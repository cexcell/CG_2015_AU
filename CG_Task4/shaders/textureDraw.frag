#version 430

layout (binding = 0) uniform sampler2D gPosition;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D gAlbedoSpec;
layout (binding = 3) uniform sampler2D lightColor;

in vec2 TexCoords;
out vec4 FragColor;

uniform int mode;
uniform int enableGamma;

void main()
{
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
	float Specular = texture(gAlbedoSpec, TexCoords).a;
	
	if (mode == 1)
		FragColor = vec4(FragPos, 1.0f);
	if (mode == 2)
		FragColor = vec4(Normal, 1.0f);
	if (mode == 3)
		FragColor = vec4(Diffuse, 1.0f);
	if (mode == 4)
		FragColor = vec4(Specular, Specular, Specular, 1.0f);
	if (mode == 5)
	{
		float gamma = 2.2;
		FragColor = texture(lightColor, TexCoords);
		if (enableGamma > 0)
			FragColor.rgb = pow(FragColor.rgb, vec3(1.0/gamma));
	}
}