#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

out vec3 TexCoords;

void main()
{
    vec4 pos = projectionMatrix * mat4(mat3(viewMatrix)) * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
    
    TexCoords = aPos;
}