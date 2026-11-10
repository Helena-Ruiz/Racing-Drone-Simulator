#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 WorldPos;
in vec3 Normal;
in vec3 Tangent;
in vec3 Bitangent;

// --- TEXTURES ---
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicRoughnessMap; // GLTF: g=roughness, b=metallic
uniform sampler2D aoMap;
uniform sampler2D emissiveMap;          

// --- FLAGS ---
uniform bool useAlbedoMap;
uniform bool useNormalMap;
uniform bool useMetallicRoughnessMap;
uniform bool useAOMap;
uniform bool useEmissiveMap;            

// --- VALORS PER DEFECTE ---
uniform vec4  defaultAlbedo;
uniform float defaultMetallic;
uniform float defaultRoughness;
uniform int alphaMode;
uniform float alphaCutoff;
uniform vec3  defaultEmissive;          
uniform float emissiveIntensity;        

// --- LLUMS ---
uniform vec3 camPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float lightIntensity;

// --- GLOW PARAMS ---
uniform float glowStrength;     
uniform float glowSize;         

// --- SHADOWS ---
uniform sampler2DShadow shadowMap;
in vec4 FragPosLightSpace;

// --- DEBUG ---
uniform int debugMode;

const float PI = 3.14159265359;

// --------------------------------------------------------------------
// FUNCIONS PBR BÀSIQUES
// --------------------------------------------------------------------

float ShadowCalculation(vec4 fragPosLightSpace, vec3 N, vec3 L)
{
    // 1. Divisió perspectiva
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // 2. Transformar a [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    // 3. Fora del mapa = no ombra
    if(projCoords.z > 1.0) return 0.0;

    // 4. Bias reduït per al dron
    float bias = max(0.001 * (1.0 - dot(N, L)), 0.0005);

    // 5. HARDWARE PCF
    float shadow = 0.0;

    // Mostreig PCF
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            // texture() amb sampler2DShadow ja retorna la comparació (0.0 o 1.0 interpolat)
            shadow += texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, projCoords.z - bias)); 
        }    
    }
    shadow /= 9.0;

    // Retorna 1.0 si està IL·LUMINAT (no hi ha ombra), 0.0 si està en OMBRA TOTAL.
    // Invertim el resultat per seguir la lògica del main (0 = Llum, 1 = Ombra)
    // PERÒ ATENCIÓ: Al codi original el return era "1.0 - shadow".
    // Si texture() retorna 1.0 quan Z < Depth (Il·luminat):
    // shadow = 1.0 -> return 0.0 (No hi ha factor d'ombra a restar)
    // Si texture() retorna 0.0 quan Z > Depth (Ombra):
    // shadow = 0.0 -> return 1.0 (Factor d'ombra màxim)
    
    return 1.0 - shadow; 
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return a2 / max(denom, 0.000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// --------------------------------------------------------------------
void main()
{
    // --------------------------------------------------------------------
    // 1. OBTENIR DADES DE LES TEXTURES
    // --------------------------------------------------------------------
    
    // Albedo (sRGB -> Linear)
    vec4 albedoRGBA = defaultAlbedo;
    if (useAlbedoMap) {
        albedoRGBA *= texture(albedoMap, TexCoord);
    }
    
    float alpha = albedoRGBA.a;

    if (alphaMode == 1) { // MASK
        if (alpha < alphaCutoff) discard;
    }

    // Convert to linear
    vec3 albedo = pow(albedoRGBA.rgb, vec3(2.2));
    
    // Metallic i Roughness
    float metallic = defaultMetallic;
    float roughness = defaultRoughness;
    
    if (useMetallicRoughnessMap) {
        vec3 mrSample = texture(metallicRoughnessMap, TexCoord).rgb;
        roughness = clamp(roughness * mrSample.g, 0.04, 1.0);
        metallic = clamp(metallic * mrSample.b, 0.0, 1.0);
    }
    
    // AO
    float ao = 1.0;
    if (useAOMap) {
        ao = texture(aoMap, TexCoord).r;
    }
    
    // Emissive (sRGB -> Linear)
    vec3 emission = defaultEmissive;
    if (useEmissiveMap) {
        vec3 e = texture(emissiveMap, TexCoord).rgb;
        // Si la textura emissive és sRGB, converteix a linear o assegura't del format.
        e = pow(e, vec3(2.2)); // només si la texel no està ja enlinear
        emission *= e * emissiveIntensity;
    }
    
    // Normal
    vec3 N = normalize(Normal);
    if (useNormalMap) {
        vec3 normalSample = texture(normalMap, TexCoord).rgb;
        normalSample = normalSample * 2.0 - 1.0;
        mat3 TBN = mat3(normalize(Tangent), normalize(Bitangent), N);
        N = normalize(TBN * normalSample);
    }
    
    // Double sided fix
    if (!gl_FrontFacing) N = -N;

    // PRE-CÀLCUL DE VECTORS (Necessaris per al debug d'ombres)
    vec3 V = normalize(camPos - WorldPos);
    vec3 L = normalize(-lightDir); // Llum direccional

    // --------------------------------------------------------------------
    // 2. MODES DEBUG
    // --------------------------------------------------------------------
    if (debugMode > 0) {
        vec3 debugColor;
        
        switch(debugMode) {
            case 1: // Albedo
                debugColor = albedo;
                break;
            case 2: // Normal
                debugColor = N * 0.5 + 0.5;
                break;
            case 3: // Metallic
                debugColor = vec3(metallic);
                break;
            case 4: // Roughness
                debugColor = vec3(roughness);
                break;
            case 5: // AO
                debugColor = vec3(ao);
                break;
            case 6: // UV Checker
                vec2 grid = floor(TexCoord * 8.0);
                if (mod(grid.x + grid.y, 2.0) < 0.5)
                    debugColor = vec3(1.0, 0.0, 0.0);
                else
                    debugColor = vec3(0.0, 0.0, 1.0);
                break;
            case 7: // Emissive
                debugColor = emission;
                break;
            case 8: // SHADOW MAP DEBUG (NOU)
                // Calculem quanta ombra rep aquest fragment
                // ShadowCalculation retorna 1.0 si és fosc (ombra) i 0.0 si és clar.
                float shadowFactor = ShadowCalculation(FragPosLightSpace, N, L);
                
                // Visualització: Blanc = Il·luminat, Negre = Ombra
                // Invertim el valor perquè sigui intuïtiu (Llum = Blanc)
                debugColor = vec3(1.0 - shadowFactor); 
                break;
            default:
                debugColor = vec3(1.0, 0.0, 1.0);
        }
        
        // Convertir a sRGB per als modes de color
        if (debugMode == 1 || debugMode == 6 || debugMode == 7) {
            debugColor = pow(debugColor, vec3(1.0/2.2));
        }
        
        FragColor = vec4(debugColor, albedoRGBA.a);
        return;
    }
    
    // --------------------------------------------------------------------
    // 3. CÀLCUL PBR
    // --------------------------------------------------------------------
    
    // F0: reflectivitat base
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    
    vec3 Lo = vec3(0.0);
    
    if (NdotL > 0.0) {
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        // Specular
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
        vec3 specular = numerator / denominator;
        
        // Conservació d'energia
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        // Radiança
        vec3 radiance = lightColor * lightIntensity * NdotL;
        
        // Contribució directa
        // Lo = (kD * albedo / PI + specular) * radiance;

        // Calcular ombra
        float shadow = ShadowCalculation(FragPosLightSpace, N, L);

        // Multiplicar la llum directa pel factor d'ombra (1.0 - shadow)
        Lo = (1.0 - shadow) * (kD * albedo / PI + specular) * radiance;
    }
    
    // --------------------------------------------------------------------
    // 4. AMBIENT 
    // --------------------------------------------------------------------
    
    vec3 skyColor = vec3(0.5, 0.7, 1.0) * 0.3;
    vec3 groundColor = vec3(0.1, 0.1, 0.1);
    float up = N.y * 0.5 + 0.5;
    vec3 ambientColor = mix(groundColor, skyColor, up);
    
    ambientColor *= ao;
    
    float NdotV = max(dot(N, V), 0.0);
    vec3 F_ambient = fresnelSchlick(NdotV, F0);
    vec3 kS_ambient = F_ambient;
    vec3 kD_ambient = 1.0 - kS_ambient;
    kD_ambient *= 1.0 - metallic;
    
    vec3 ambient = (kD_ambient * albedo + kS_ambient) * ambientColor * 0.5;
    
    // --------------------------------------------------------------------
    // 5. COMBINACIÓ FINAL 
    // --------------------------------------------------------------------
    
    vec3 color = ambient + Lo;
    color += emission;
    
    // Tone mapping
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, alpha);
}