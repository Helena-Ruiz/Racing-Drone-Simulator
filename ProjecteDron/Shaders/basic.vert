#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aTangent;

out vec2 TexCoord;
out vec3 WorldPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 Bitangent;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;


void main()
{
    WorldPos = vec3(model * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalize(normalMatrix * aNormal);
    
    Tangent = normalize(normalMatrix * aTangent.xyz);
    Bitangent = cross(Normal, Tangent) * aTangent.w;
    
    FragPosLightSpace = lightSpaceMatrix * vec4(WorldPos, 1.0);

    gl_Position = projection * view * vec4(WorldPos, 1.0);
}