#version 330

uniform vec3 sphereColor;

void main()
{
	gl_FragColor = vec4(sphereColor, 1.0f);
}