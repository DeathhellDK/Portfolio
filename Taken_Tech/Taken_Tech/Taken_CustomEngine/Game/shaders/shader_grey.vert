#version 330 core

// Input attributes
layout(location = 0) in vec2 aPos;    // position
layout(location = 1) in vec2 aUV;     // texture coordinates 
layout(location = 2) in vec3 aColor;  // vertex color

// Uniforms
uniform mat3 u_MVP;   // model-view-projection 

/* sprite-sheet animation uniforms */
uniform int   u_Frame;       // current frame index
uniform vec2  u_FrameSize;   // (frameW/texW, frameH/texH)
uniform int   u_Cols;        // columns in the sheet

// Pass to fragment
out vec3 vColor;
out vec2 vUV;

void main() {
    vec3 worldPos = u_MVP * vec3(aPos, 1.0);
    gl_Position = vec4(worldPos.xy, 0.0, 1.0);

    // compute per-frame UV offset
    int cx = u_Frame % u_Cols;
    int cy = u_Frame / u_Cols;
    vec2 offset = vec2(cx, cy) * u_FrameSize;

    // slight dim 
    vColor = aColor * 0.5;
    vUV    = aUV * u_FrameSize + offset;
}