#version 440
layout(location = 0) in vec2 position;
layout(location = 0) out vec2 v_texCoord;

void main() {
    v_texCoord = (position + 1.0) * 0.5;
    // 翻转 Y 轴适配 Shadertoy 坐标系 (Shadertoy 原点在左下角)
    v_texCoord.y = 1.0 - v_texCoord.y;
    gl_Position = vec4(position, 0.0, 1.0);
}
