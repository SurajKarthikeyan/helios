#version 330 core
in vec2 uv;
in vec3 norm;
in vec3 fragpos;
out vec4 fragColor;
uniform sampler2D earth_tex;
uniform vec3 light_dir;
uniform vec3 tint_color;
void main() {
    vec3 tex = texture(earth_tex, uv).rgb;
    vec3 n = normalize(norm);
    vec3 l = normalize(light_dir);
    float diff = max(dot(n, l), 0.0);
    float ambient = 0.4;
    float land = step(tex.b * 1.8, tex.r + tex.g);
    float brightness = ambient + diff * 0.8;
    vec3 land_col = tint_color * brightness;
    vec3 water_col = vec3(0.02, 0.02, 0.02);
    vec3 col = mix(water_col, land_col, land);
    float rim = 1.0 - max(dot(n, normalize(vec3(0,0,1))), 0.0);
    rim = pow(rim, 3.0);
    col += tint_color * rim * 0.4;
    fragColor = vec4(col, 1.0);
}
