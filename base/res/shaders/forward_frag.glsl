#version 430 compatibility

in vec3 fs_Normal;
in vec4 fs_Position;
in vec4 fs_LPosition;
in vec3 fs_WNormal;

out vec4 outColor;

uniform mat4 u_ViewInverse;
uniform sampler2D u_ShadowMap;
uniform vec3 u_Color;
uniform int u_numLights;
uniform int u_numVPLs;

struct	LightData
{
	vec4	position;
	vec4	intensity;
};

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
	vec4 wPosition = u_ViewInverse * fs_Position;
	vec3 lColor = vec3 (1.0);
	vec3 outColor3 = vec3 (0.0, 0.0, 0.0);
	vec4 lightVec = vec4 (0.0);
	
	for (int i = 0; i < u_numLights; ++i)
	{
		lightVec = vec4(lights [i].xyz, 1.0) - wPosition;
		float clampedDiffuseFactor = clamp (dot (fs_WNormal, normalize(lightVec.xyz)), 0.0, 1.0);
		outColor3 += (u_Color * clampedDiffuseFactor);		
	}

	int count = 0;
	for (int i = 0; i < u_numVPLs; ++i)
	{
		if (length (vpl [i].intensity.xyz) < 0.001)
			continue;
		lightVec = vpl [i].position - wPosition;
		float clampedDiffuseFactor = clamp (dot (fs_WNormal, normalize (lightVec.xyz)), 0.0, 1.0);
		outColor3 += (u_Color * clampedDiffuseFactor * vpl [i].intensity.xyz * vpl [i].intensity.w);
		++ count;
	}

	outColor3 /= 8.0;
	outColor = vec4 (outColor3, 1.0);
}