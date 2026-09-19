#version 330 core
in vec2 vUV;
in vec3 vColor;
out vec4 FragColor;

uniform sampler2D u_Tex;
uniform bool  u_UseTexture;
uniform vec3  u_Tint;   // optional tint
uniform float u_Alpha;

void main() {
    vec4 tex = u_UseTexture ? texture(u_Tex, vUV) : vec4(1.0);
    FragColor = tex * vec4(vColor * u_Tint, 1.0);
    FragColor.a *= u_Alpha;
}