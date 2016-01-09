#version 330 core

layout(location = 0) in vec3 position;

uniform mat4 InvProjProjection;
uniform mat4 InvProjModelView;
uniform mat4 Projection;
uniform mat4 ModelView;

void main()
{
    vec4 fVp = InvProjModelView * InvProjProjection * vec4(position, 1.0);
    gl_Position = Projection * ModelView * fVp;
}