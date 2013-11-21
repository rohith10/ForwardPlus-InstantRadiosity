#version 330

in vec3 Position;
in vec2 Texcoord;

out vec2 fs_Texcoord;

void main() {
    fs_Texcoord = Texcoord;
    gl_Position = vec4(Position,1.0f);
}
