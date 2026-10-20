#pragma once

#include <string>
#include <glm/glm.hpp>
#include "../core/Types.h"

namespace Orbital {

enum class PlanetBiomeType {
    Terrestrial,  // Oceans, green continents, mountain ridges, polar ice caps
    Desert,       // Mars-like iron oxide sands, rocky crags, dust storm hazes
    OceanWorld,   // Endless deep sapphire ocean, swirling storm clouds
    IceWorld,     // Frozen ice crust, blue glacial fissures, cryovolcanism
    Volcanic,     // Basalt plains, sulfur fields, glowing molten lava rivers
    GasGiant      // Zonal latitudinal wind bands, turbulent storm vortices
};

inline const char* GetBiomeTypeName(PlanetBiomeType b) {
    switch (b) {
        case PlanetBiomeType::Terrestrial: return "Terrestrial (Earth-like)";
        case PlanetBiomeType::Desert:      return "Desert (Mars-like)";
        case PlanetBiomeType::OceanWorld:  return "Ocean World";
        case PlanetBiomeType::IceWorld:    return "Ice World (Europa-like)";
        case PlanetBiomeType::Volcanic:    return "Volcanic (Io-like)";
        case PlanetBiomeType::GasGiant:    return "Gas Giant (Jupiter-like)";
    }
    return "Unknown";
}

struct ProceduralPlanetSurface {
    // Advanced procedural multi-octave simplex/perlin noise shader with biomes,
    // ocean specular masks, cloud shadow projection, and atmospheric scattering
    static const char* getVertexShader() {
        return R"(#version 430 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;

        out vec3 FragPosView;
        out vec3 NormalView;
        out vec2 TexCoord;
        out vec3 ObjectPos;
        out vec3 WorldNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main()
        {
            ObjectPos = aPos;
            WorldNormal = aNormal;
            vec4 viewPos = view * model * vec4(aPos, 1.0);
            FragPosView = viewPos.xyz;
            NormalView = mat3(transpose(inverse(view * model))) * aNormal;
            TexCoord = aTexCoord;
            gl_Position = projection * viewPos;
        }
        )";
    }

    static const char* getFragmentShader() {
        return R"(#version 430 core
        out vec4 FragColor;

        in vec3 FragPosView;
        in vec3 NormalView;
        in vec2 TexCoord;
        in vec3 ObjectPos;
        in vec3 WorldNormal;

        // Uniforms
        uniform vec3 baseColor;
        uniform float emission;
        uniform bool isBlackHole;
        uniform bool isGasGiant;
        uniform int biomeType; // 0=Terrestrial, 1=Desert, 2=Ocean, 3=Ice, 4=Volcanic, 5=GasGiant
        uniform float seed;
        uniform float simTime;

        uniform bool hasAtmosphere;
        uniform vec3 atmosphereColor;
        uniform float atmosphereDensity;

        uniform float seaLevel;       // Default ~ 0.50
        uniform float cloudCoverage;  // Default ~ 0.45

        // Simplex/Perlin noise helper functions
        vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
        vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
        vec4 permute(vec4 x) { return mod289(((x*34.0)+1.0)*x); }
        vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

        float snoise(vec3 v) {
            const vec2 C = vec2(1.0/6.0, 1.0/3.0);
            const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

            vec3 i  = floor(v + dot(v, C.yyy));
            vec3 x0 = v - i + dot(i, C.xxx);

            vec3 g = step(x0.yzx, x0.xyz);
            vec3 l = 1.0 - g;
            vec3 i1 = min(g.xyz, l.zxy);
            vec3 i2 = max(g.xyz, l.zxy);

            vec3 x1 = x0 - i1 + C.xxx;
            vec3 x2 = x0 - i2 + C.yyy;
            vec3 x3 = x0 - D.yyy;

            i = mod289(i);
            vec4 p = permute(permute(permute(
                        i.z + vec4(0.0, i1.z, i2.z, 1.0))
                    + i.y + vec4(0.0, i1.y, i2.y, 1.0))
                    + i.x + vec4(0.0, i1.x, i2.x, 1.0));

            float n_ = 0.142857142857;
            vec3 ns = n_ * D.wyz - D.xzx;

            vec4 j = p - 49.0 * floor(p * ns.z * ns.z);

            vec4 x_ = floor(j * ns.z);
            vec4 y_ = floor(j - 7.0 * x_);

            vec4 x = x_ *ns.x + ns.yyyy;
            vec4 y = y_ *ns.x + ns.yyyy;
            vec4 h = 1.0 - abs(x) - abs(y);

            vec4 b0 = vec4(x.xy, y.xy);
            vec4 b1 = vec4(x.zw, y.zw);

            vec4 s0 = floor(b0)*2.0 + 1.0;
            vec4 s1 = floor(b1)*2.0 + 1.0;
            vec4 sh = -step(h, vec4(0.0));

            vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy;
            vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww;

            vec3 p0 = vec3(a0.xy, h.x);
            vec3 p1 = vec3(a0.zw, h.y);
            vec3 p2 = vec3(a1.xy, h.z);
            vec3 p3 = vec3(a1.zw, h.w);

            vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
            p0 *= norm.x;
            p1 *= norm.y;
            p2 *= norm.z;
            p3 *= norm.w;

            vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
            m = m * m;
            return 42.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
        }

        // Fractal Brownian Motion (fBm)
        float fbm(vec3 p, int octaves) {
            float v = 0.0;
            float a = 0.5;
            vec3 shift = vec3(100.0);
            for (int i = 0; i < octaves; ++i) {
                v += a * snoise(p);
                p = p * 2.05 + shift;
                a *= 0.5;
            }
            return v;
        }

        void main()
        {
            // 1. Black Hole event horizon & photon ring
            if (isBlackHole) {
                vec3 N = normalize(NormalView);
                vec3 V = normalize(-FragPosView);
                float rim = 1.0 - max(dot(N, V), 0.0);
                float photonRing = pow(rim, 7.0);
                vec3 ringGlow = vec3(1.0, 0.82, 0.45) * photonRing * 3.5;
                FragColor = vec4(ringGlow, 1.0);
                return;
            }

            // 2. Stars (Atmospheric self-luminous limb darkening)
            if (emission > 0.5) {
                vec3 N = normalize(NormalView);
                vec3 V = normalize(-FragPosView);
                float mu = max(dot(N, V), 0.0);
                // Eddington solar limb-darkening approximation: I(mu) = I_0 * (0.4 + 0.6 * mu)
                float limbDarkening = 0.4 + 0.6 * mu;
                
                // Turbulent solar convective granulation
                vec3 sunCoord = ObjectPos * 8.0 + vec3(simTime * 0.002);
                float granules = snoise(sunCoord) * 0.08;

                vec3 starColor = baseColor * (limbDarkening + granules);
                FragColor = vec4(starColor * 1.3, 1.0);
                return;
            }

            // 3. Coordinate normalization for procedural sampling
            vec3 normPos = normalize(ObjectPos);
            float latitude = abs(normPos.y); // 0 at equator, 1 at poles

            // 4. Gas Giant procedural generation
            if (biomeType == 5 || isGasGiant) {
                float y = ObjectPos.y * 14.0;
                float turb = fbm(ObjectPos * 3.5 + vec3(0.0, 0.0, simTime * 0.0005), 4) * 0.45;
                float bandPattern = sin(y + turb * 3.0);

                // Alternating atmospheric zones & belts
                vec3 zoneCol = baseColor * 1.15;
                vec3 beltCol = baseColor * 0.65 + vec3(0.12, 0.04, -0.05);
                vec3 gasCol = mix(beltCol, zoneCol, smoothstep(-0.4, 0.4, bandPattern));

                // Great Red Spot / storm vortex approximation
                vec3 stormCoord = (ObjectPos - vec3(0.4, -0.25, 0.4)) * 6.0;
                float stormDist = length(stormCoord);
                if (stormDist < 1.0) {
                    float stormSpiral = snoise(stormCoord * 2.0);
                    gasCol = mix(vec3(0.85, 0.30, 0.15), gasCol, smoothstep(0.4, 0.9, stormDist + stormSpiral * 0.15));
                }

                // Diffuse lighting
                vec3 N = normalize(NormalView);
                vec3 L = normalize(vec3(0.5, 0.3, 0.8));
                float diff = max(dot(N, L), 0.05);
                FragColor = vec4(gasCol * diff, 1.0);
                return;
            }

            // 5. Solid / Terrestrial planet terrain generation
            vec3 seedOffset = vec3(seed * 17.3, seed * 31.7, seed * 43.1);
            vec3 samplePos = normPos * 2.8 + seedOffset;

            // Continent elevation map
            float elevation = fbm(samplePos, 5) * 0.5 + 0.5; // 0.0 to 1.0
            
            // Polar ice cap mask
            float iceFactor = smoothstep(0.72, 0.92, latitude + snoise(samplePos * 4.0) * 0.08);

            vec3 surfaceAlbedo = baseColor;
            float isWater = 0.0;
            float specularPower = 4.0;
            vec3 specColor = vec3(0.04);

            if (biomeType == 0) {
                // Terrestrial (Earth-like): Ocean, Sand, Plains, Forest, Mountains, Snow
                float actualSea = seaLevel > 0.0 ? seaLevel : 0.48;
                if (elevation < actualSea) {
                    // Ocean: Depth gradient from deep navy to coastal cyan
                    float depth = elevation / actualSea;
                    surfaceAlbedo = mix(vec3(0.02, 0.08, 0.28), vec3(0.05, 0.32, 0.52), depth);
                    isWater = 1.0;
                    specularPower = 64.0;
                    specColor = vec3(0.85, 0.92, 1.0); // Bright ocean sun glint
                } else {
                    float landHeight = (elevation - actualSea) / (1.0 - actualSea);
                    if (landHeight < 0.08) {
                        surfaceAlbedo = vec3(0.78, 0.72, 0.52); // Coastal sand/beach
                    } else if (landHeight < 0.45) {
                        surfaceAlbedo = mix(vec3(0.22, 0.52, 0.18), vec3(0.12, 0.38, 0.12), landHeight * 2.0); // Grass/forest
                    } else if (landHeight < 0.75) {
                        surfaceAlbedo = mix(vec3(0.42, 0.38, 0.32), vec3(0.30, 0.26, 0.22), (landHeight - 0.45) / 0.3); // Mountain rock
                    } else {
                        surfaceAlbedo = vec3(0.92, 0.94, 0.96); // Mountain peak snow
                    }
                }
            } else if (biomeType == 1) {
                // Desert (Mars-like): Rust iron oxide, orange dunes, dark basalt crags
                float duneNoise = fbm(samplePos * 4.0, 4);
                vec3 sandColor = vec3(0.78, 0.38, 0.18);
                vec3 cragColor = vec3(0.45, 0.20, 0.12);
                surfaceAlbedo = mix(sandColor, cragColor, smoothstep(0.35, 0.65, elevation + duneNoise * 0.2));
            } else if (biomeType == 2) {
                // Ocean World: Endless sapphire waters
                surfaceAlbedo = mix(vec3(0.01, 0.06, 0.35), vec3(0.04, 0.28, 0.58), elevation);
                isWater = 1.0;
                specularPower = 64.0;
                specColor = vec3(0.9, 0.95, 1.0);
            } else if (biomeType == 3) {
                // Ice World: Europa-like cracked ice crust
                float crack = abs(snoise(samplePos * 8.0));
                vec3 iceColor = vec3(0.92, 0.95, 0.99);
                vec3 crevasse = vec3(0.18, 0.45, 0.65);
                surfaceAlbedo = mix(crevasse, iceColor, smoothstep(0.05, 0.25, crack));
                specularPower = 32.0;
                specColor = vec3(0.5);
            } else if (biomeType == 4) {
                // Volcanic / Io: Sulfur plains and glowing magma
                float lavaRift = smoothstep(0.85, 0.95, fbm(samplePos * 6.0, 3));
                vec3 basalt = vec3(0.18, 0.16, 0.15);
                vec3 sulfur = vec3(0.85, 0.75, 0.15);
                surfaceAlbedo = mix(sulfur, basalt, elevation);
                if (lavaRift > 0.05) {
                    vec3 magma = vec3(1.0, 0.35, 0.05) * 3.0; // Emissive magma
                    surfaceAlbedo = mix(surfaceAlbedo, magma, lavaRift);
                }
            }

            // Apply polar ice caps
            if (iceFactor > 0.0 && biomeType != 4) {
                surfaceAlbedo = mix(surfaceAlbedo, vec3(0.95, 0.96, 0.99), iceFactor);
                isWater = 0.0;
                specularPower = 24.0;
            }

            // 6. Dynamic procedural clouds
            float clouds = 0.0;
            if (cloudCoverage > 0.05 && biomeType != 1 && biomeType != 4) {
                vec3 cloudPos = normPos * 3.2 + vec3(simTime * 0.0003, 0.0, seed * 5.0);
                float cloudNoise = fbm(cloudPos, 4);
                clouds = smoothstep(1.0 - cloudCoverage, 1.0 - cloudCoverage + 0.25, cloudNoise);
                
                // Cloud shadow on terrain below
                vec3 shadowPos = normPos * 3.2 + vec3(simTime * 0.0003 + 0.02, 0.0, seed * 5.0);
                float shadow = smoothstep(1.0 - cloudCoverage, 1.0 - cloudCoverage + 0.25, fbm(shadowPos, 4));
                surfaceAlbedo *= (1.0 - shadow * 0.4);

                // Blend white clouds onto surface
                surfaceAlbedo = mix(surfaceAlbedo, vec3(0.96, 0.97, 1.0), clouds);
            }

            // 7. Lighting Model (Key Sunlight + View + Specular + Atmosphere)
            vec3 N = normalize(NormalView);
            vec3 L = normalize(vec3(0.5, 0.3, 0.8)); // Key light in view space
            vec3 V = normalize(-FragPosView);
            vec3 H = normalize(L + V);

            float diff = max(dot(N, L), 0.0);
            float diffWrap = max((dot(N, L) + 0.12) / 1.12, 0.0); // Gentle wrap around terminator
            vec3 ambient = vec3(0.03, 0.04, 0.07) * surfaceAlbedo;
            vec3 diffuse = diffWrap * vec3(1.0, 0.98, 0.94) * surfaceAlbedo;

            // Specular sunlight glint (water only)
            vec3 specular = vec3(0.0);
            if (isWater > 0.5 && clouds < 0.2) {
                float spec = pow(max(dot(N, H), 0.0), specularPower);
                specular = spec * specColor * diff;
            }

            // 8. Rayleigh & Mie Atmospheric Limb Scattering
            vec3 atmoGlow = vec3(0.0);
            if (hasAtmosphere) {
                float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.2);
                // Rayleigh day limb (blue) with sunset/terminator reddening
                float terminatorSun = clamp(dot(N, L), -0.2, 0.3);
                vec3 sunsetColor = vec3(1.0, 0.45, 0.15);
                vec3 dayColor = atmosphereColor;
                vec3 scatColor = mix(sunsetColor, dayColor, smoothstep(-0.1, 0.25, dot(N, L)));

                atmoGlow = fresnel * scatColor * atmosphereDensity * max(dot(N, L) + 0.2, 0.0);
            }

            vec3 finalColor = ambient + diffuse + specular + atmoGlow;
            FragColor = vec4(finalColor, 1.0);
        }
        )";
    }
};

} // namespace Orbital
