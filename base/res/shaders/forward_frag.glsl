#version 430 compatibility

in vec3 fs_Normal;
in vec4 fs_Position;
in vec4 fs_LPosition;
in vec3 fs_WNormal;

out vec4 outColor;

uniform mat4 u_ViewInverse
uniform sampler2D u_ShadowMap;
uniform vec3 u_Color;
uniform int u_numLights;
uniform int u_numVPLs;

layout (std430, binding=1) buffer vplInfo
{
	LightData vpl[];
};

layout (std430, binding=2) buffer lightInfo
{
	vec4	lights[];
};

void main ()
{
	vec4 wPosition = u_viewInverse * fs_Position;
	vec3 lColor = vec3 (1.0);
	outColor = vec4 (0.0, 0.0, 0.0, 1.0);
	vec4 lightVec = vec4 (0.0);
	
	for (int i = 0; i < u_numLights; ++i)
	{
		lightVec = normalize (lights [i] - wPosition);
		float clampedDiffuseFactor = clamp (dot (fs_WNormal, lightVec), 0.0, 1.0);
		outVec += (u_Color * clampedDiffuseFactor);
	}

	for (int i = 0; i < u_numVPLs; ++i)
	{
		lightVec = normalize (vpl [i] - wPosition);
		float clampedDiffuseFactor = clamp (dot (fs_WNormal, lightVec), 0.0, 1.0);
		outVec += (u_Color * clampedDiffuseFactor);
	}
}