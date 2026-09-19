#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform vec3  u_LightColor;
uniform float u_Intensity;
uniform float u_Opacity;
uniform float u_Attenuation; // used for source: 1/(1 + k*r^2)
uniform float u_Softness;    // used for glow: pow(max(1-r,0), softness)
uniform int   u_Mode;        // 0 = Glow, 1 = Source

void main() {
    // vUV is [0..1], center assumed at 0.5,0.5 for unit circle mesh
    float r = length(vUV - vec2(0.5));
    // normalize to [0..1] radius (unit circle mesh uses 0.5 radius)
    float rn = clamp(r * 2.0, 0.0, 1.0);

    float glow = pow(max(1.0 - rn, 0.0), max(u_Softness, 0.0));
    // Multiply by squared falloff to ensure it reaches 0 at the mesh edge (rn=1) smoothly
    float source = (1.0 / (1.0 + max(u_Attenuation, 0.0) * rn * rn)) * pow(1.0 - rn, 2.0);

    float falloff = (u_Mode == 0) ? glow : source;
    float alpha = clamp(u_Intensity * u_Opacity * falloff, 0.0, 1.0);

    vec3 rgb = u_LightColor * alpha;
    FragColor = vec4(rgb, alpha);
}
