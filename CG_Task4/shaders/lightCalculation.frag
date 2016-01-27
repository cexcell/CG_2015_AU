#version 430

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D gPosition;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D gAlbedoSpec;

struct DirLight 
{
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform DirLight dirLight;

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

const int maxLights = 256;
uniform int lightsN;
uniform PointLight pointLights[maxLights];

uniform vec3 viewPos;
uniform int mode;

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

vec3 calculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 Diffuse, float Specular)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
	float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);

	vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
	vec3 ambient  = light.ambient  * Diffuse;
    vec3 diffuse  = light.diffuse  * diff * Diffuse;
    vec3 specular = light.specular * spec * Specular;
    diffuse  *= attenuation;
    specular *= attenuation;
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

	if (mode == 1)
		gl_FragColor = vec4(FragPos, 1.0f);
	if (mode == 2)
		gl_FragColor = vec4(Normal, 1.0f);
	if (mode == 3)
		gl_FragColor = vec4(Diffuse, 1.0f);
	if (mode == 4)
		gl_FragColor = vec4(Specular, Specular, Specular, 1.0f);
	if (mode == 5)
	{
		vec3 color;
		
		color = calculateDirectionalLight(dirLight, norm, viewDir, Diffuse, Specular);
		for(int i = 0; i < lightsN; i++)
			color += calculatePointLight(pointLights[i], norm, FragPos, viewDir, Diffuse, Specular);    
		gl_FragColor = vec4(color, 1.0f);
	}
}