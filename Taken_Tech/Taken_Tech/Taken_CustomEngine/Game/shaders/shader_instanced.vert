

#version 330 core

layout (location=0) in vec2 a_Pos;
layout (location=1) in vec2 a_UV;
layout (location=2) in vec3 a_ColorIgnored; // still in mesh, but ignore it

// Per-instance transform (mat3 as 3 vec3 columns)
layout (location=3) in vec3 a_m0;
layout (location=4) in vec3 a_m1;
layout (location=5) in vec3 a_m2;

// Per-instance color gradient + age
layout (location=6) in vec3 a_ColorStart;
layout (location=7) in vec3 a_ColorEnd;
layout (location=8) in float a_Age01; // 0 = birth, 1 = end of life

uniform mat3 u_VP; // camera/view/proj for 2D

out vec2 v_UV;
out vec3 v_LerpedColor;

void main() {
    // rebuild model matrix
    mat3 model = mat3(a_m0, a_m1, a_m2);

    // world position of this vertex
    vec3 world = model * vec3(a_Pos, 1.0);

    // clip position
    vec3 clip  = u_VP * world;
    gl_Position = vec4(clip.xy, 0.0, 1.0);

    // pass UV down
    v_UV = a_UV;

    // do lifetime color interpolation on GPU
    v_LerpedColor = mix(a_ColorStart, a_ColorEnd, a_Age01);
}
