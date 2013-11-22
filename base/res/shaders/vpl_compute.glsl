#version 430 compatibility
#extension	GL_ARB_compute_shader:					enable
#extension	GL_ARB_shader_storage_buffer_object:	enable

struct	LightData
{
	vec3	position;
	float	intensity;
};

layout (std140, binding = 1) buffer lightPos
{
	LightData lights [];
};

uniform int u_numLights;

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main(void)
{
    ;
}
