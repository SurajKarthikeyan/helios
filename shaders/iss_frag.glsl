#version 330 core
out vec4 fragColor;
uniform vec3 tint_color;
void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float r = length(coord);
    if (r > 0.5) discard;
    float core = 1.0 - smoothstep(0.0, 0.2, r);
    float glow = 1.0 - smoothstep(0.1, 0.5, r);
    vec3 col = mix(vec3(1.0), tint_color, 0.3) * core;
    col += tint_color * glow * 0.6;
    fragColor = vec4(col, glow);
}
