#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 WorldPos;

uniform vec3 color;      // Color base (Verd, Vermell, etc.)
uniform float time;      // Per fer l'animació de pulsació
uniform vec3 camPos;     // Per calcular l'angle de visió

void main()
{
    // 1. Normalitzar vectors
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(camPos - WorldPos);

    // 2. Efecte FRESNEL (Vores brillants)
    // Calcula l'angle entre la vista i la superfície. 
    // Si ets perpendicular (centre), és fosc. Si és rasant (vores), és brillant.
    float fresnel = dot(viewDir, norm);
    fresnel = clamp(1.0 - fresnel, 0.0, 1.0);
    fresnel = pow(fresnel, 2.5); // Ajusta la potència per fer-lo més fi o gruixut

    // 3. Efecte PULSACIÓ (respira)
    float pulse = (sin(time * 3.0) * 0.5 + 0.5); // Oscil·la entre 0.0 i 1.0
    
    // 4. Combinar tot
    // La base és semitransparent, les vores són molt brillants
    vec3 finalColor = color + (color * fresnel * 2.0); 
    
    // Alpha: El centre és transparent (0.1), les vores opaques (fresnel)
    // + un petit parpelleig amb el pulse
    float alpha = 0.1 + (fresnel * 0.8) + (pulse * 0.1);

    FragColor = vec4(finalColor, alpha);
}