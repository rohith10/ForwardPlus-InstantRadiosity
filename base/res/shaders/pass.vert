#version 330


uniform mat4x4 u_Model;
uniform mat4x4 u_View;
uniform mat4x4 u_Persp;
uniform mat4x4 u_InvTrans;

in  vec3 Position;
in  vec3 Normal;

out vec3 fs_Normal;
out vec4 fs_Position;

void main(void) {
    fs_Normal = (u_InvTrans*vec4(Normal,0.0f)).xyz;
    vec4 world = u_Model * vec4(Position, 1.0);
    vec4 camera = u_View * world;
    fs_Position = camera;
    gl_Position = u_Persp * camera;
}
