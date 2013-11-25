#version 330

uniform float u_Far;  
uniform float u_Near; 
uniform vec3 u_Color;
uniform float glowmask;

in vec3 fs_Normal;
in vec4 fs_Position;
in vec4 fs_LPosition;

out vec4 out_Normal;
out vec4 out_Position;
out vec4 out_Color;
out vec4 out_GlowMask;
out vec4 out_LightCoord;

uniform sampler2D u_Shadowtex;

//This restores linear depth
float linearizeDepth(float exp_depth, float near, float far) {
    return	(2 * near) / (far + near -  exp_depth * (far - near)); 
}

void main(void)
{
    out_Normal = vec4(normalize(fs_Normal),0.0f);
	out_Position = vec4(fs_Position.xyz,1.0f);

	out_LightCoord = fs_LPosition;
    if (fs_LPosition.w > 1.0f)
		out_LightCoord /= fs_LPosition.w;

	 //Tuck position into 0 1 range
    out_Color = vec4(u_Color,1.0);
	out_GlowMask = glowmask * out_Color;
}
