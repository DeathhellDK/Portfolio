#version 330 core
in vec2 vUV;
in vec3 vColor;
out vec4 FragColor;

uniform sampler2D u_Tex;
uniform bool  u_UseTexture;
uniform vec3  u_Tint;

void main() {
    vec4 base = (u_UseTexture ? texture(u_Tex, vUV) : vec4(1.0));
    base *= vec4(vColor * u_Tint, 1.0);

    // luminance (Rec. 709)
    float y = dot(base.rgb, vec3(0.2126, 0.7152, 0.0722));
    FragColor = vec4(vec3(y), base.a);
}