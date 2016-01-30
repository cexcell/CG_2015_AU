#version 430

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D gPosition;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D gAlbedoSpec;

layout (location = 0) out vec4 lightColor;

struct DirLight 
{
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform DirLight dirLight;
uniform vec3 viewPos;

vec3 calculateDirectionalLight(DirLight light, vec3 normal, vec3 viewDir, vec3 Diffuse, float Specular)
{
	vec3 lightDir = normalize(-light.direction);
    
	float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    
	vec3 ambient  = light.ambient * Diffuse;
    vec3 diffuse  = light.diffuse * diff * Diffuse;
    vec3 specular = light.specular * spec * Specular;
    return (ambient + diffuse + specular);
}

void main()
{
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
	float Specular = texture(gAlbedoSpec, TexCoords).a;

	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);

	vec3 color = calculateDirectionalLight(dirLight, norm, viewDir, Diffuse, Specular);
	lightColor = vec4(color, 1.0f);
}