#version 330 core
in vec2 uv;
out vec4 fragColor;
uniform sampler2D screen;
uniform float time;
uniform float bloom_strength;
uniform float scanline_strength;
uniform float curvature;
uniform float flicker;
void main() {
    fragColor = texture(screen, uv);
}
