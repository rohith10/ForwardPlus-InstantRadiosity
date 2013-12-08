#version 330


uniform mat4x4 u_Model;
uniform mat4x4 u_View;
uniform mat4x4 u_lView;
uniform mat4x4 u_Persp;
uniform mat4x4 u_LPersp;
uniform mat4x4 u_InvTrans;

in  vec3 Position;
in  vec3 Normal;

out vec3 fs_Normal;
out vec4 fs_Position;
out vec4 fs_LPosition;
out vec3 fs_WNormal;

void main(void) 
{
	mat4 modelToWorld = transpose (inverse (u_Model));
    fs_Normal = normalize ((u_InvTrans*vec4(Normal,0.0f)).xyz);
    fs_WNormal = normalize ((modelToWorld*vec4(Normal,0.0f)).xyz);
	vec4 world = u_Model * vec4(Position, 1.0);
	//For rendering from camera
    vec4 camera = u_View * world;
    fs_Position = camera;
	//For rendering from Light
	 vec4 lcamera = u_lView * world;
    fs_LPosition = u_LPersp * lcamera;

    gl_Position = u_Persp * camera;
}
