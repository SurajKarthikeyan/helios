#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 normal;
out vec2 uv;
out vec3 norm;
out vec3 fragpos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
void main() {
    vec4 worldpos = model * vec4(position, 1.0);
    fragpos = worldpos.xyz;
    norm = mat3(transpose(inverse(model))) * normal;
    uv = texcoord;
    gl_Position = proj * view * worldpos;
}
