#version 430

layout (binding = 0) uniform sampler2D gPosition;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D gAlbedoSpec;

layout (location = 0) out vec4 lightColor;

struct PointLight 
{    
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  

uniform PointLight pointLight;
uniform vec3 viewPos;
uniform vec2 screenSize;

vec3 calculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 Diffuse, float Specular)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
	float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);

    float distance    = length(light.position - fragPos);
    float attenuation = light.constant + light.linear * distance + light.quadratic * distance * distance;
	attenuation = max(1.0f, attenuation);
	attenuation = 1.0f / attenuation;    
    
	vec3 ambient  = light.ambient  * Diffuse;
    vec3 diffuse  = light.diffuse  * diff * Diffuse;
    vec3 specular = light.specular * spec * Specular;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

void main()
{
	vec2 TexCoords = gl_FragCoord.xy / screenSize;
	vec3 FragPos = texture(gPosition, TexCoords).rgb;
	vec3 Normal = texture(gNormal, TexCoords).rgb;
	vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
	float Specular = texture(gAlbedoSpec, TexCoords).a;

	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);

	//vec3 color = pointLight.diffuse;
	vec3 color = calculatePointLight(pointLight, norm, FragPos, viewDir, Diffuse, Specular);

	lightColor += vec4(color, 1.0);
}