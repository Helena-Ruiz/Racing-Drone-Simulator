#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 playerPos;       // Posició del dron/càmera
uniform float visibleRange;   // Distància màxima visible (ex: 50.0)
uniform float fadeWidth;      // Suavitat de la desaparició (ex: 10.0)

out float visibility;         // Factor alfa que passarem al fragment
out float height;             // Per canviar color segons alçada (opcional)

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = projection * view * worldPos;

    // --- Càlcul de distància ---
    float dist = distance(worldPos.xyz, playerPos);

    // smoothstep(min, max, valor) retorna 0 si valor < min, 1 si > max.
    // Aquí volem el contrari: 1 si està a prop, 0 si està lluny.
    
    // Si distància és 0 -> visibilitat 1.
    // Si distància és visibleRange -> comença a desaparèixer.
    // Si distància és visibleRange + fadeWidth -> totalment invisible.
    visibility = 1.0 - smoothstep(visibleRange, visibleRange + fadeWidth, dist);

    height = worldPos.y; // Passem l'alçada per fer efectes de color xulos
}