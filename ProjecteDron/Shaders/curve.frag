#version 330 core
out vec4 FragColor;

in float visibility;
in float height;

uniform float time; // Per fer que la línia "bategui" (opcional)

void main()
{
    // Si està massa lluny, descartem el píxel per estalviar
    if(visibility <= 0.01) discard;

    // --- 1. Color Base "Sci-Fi" ---
    // Barregem Cian (0.0, 1.0, 1.0) i Magenta (1.0, 0.0, 1.0) segons l'alçada
    vec3 colLow = vec3(0.0, 1.0, 1.0); // Cian
    vec3 colHigh = vec3(1.0, 0.0, 1.0); // Magenta
    
    // Normalitzem l'alçada (ajusta els valors -10 i 30 segons el teu mapa)
    float hFactor = clamp((height + 10.0) / 40.0, 0.0, 1.0);
    vec3 baseColor = mix(colLow, colHigh, hFactor);

    // --- 2. Efecte de "Pulsació" ---
    // Un petit parpelleig per fer-ho sentir digital
    float pulse = 0.8 + 0.2 * sin(time * 3.0); 

    // --- 3. Resultat Final ---
    // L'alpha es el 'visibility' calculat al vertex shader
    FragColor = vec4(baseColor * pulse, visibility);
}