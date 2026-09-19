#version 330 core

in vec2 v_UV;
in vec3 v_LerpedColor;

uniform bool      u_UseTexture;
uniform sampler2D u_Tex;
uniform vec3      u_Tint;     // global multiplier, set to 1,1,1 for now
uniform float     u_Opacity;  // set to 1.0 for now
uniform float     u_Alpha; 


out vec4 FragColor;

void main() {
    vec4 texSample = u_UseTexture ? texture(u_Tex, v_UV) : vec4(1.0);

    // final RGB = texture * lerpedColor * tint
    vec3 rgb = texSample.rgb * v_LerpedColor * u_Tint;

    float alphaTex = u_UseTexture ? texSample.a : 1.0;
    float opacity  = (u_Opacity > 0.0) ? u_Opacity : 1.0;

    FragColor = vec4(rgb, alphaTex * opacity * u_Alpha);
    
}