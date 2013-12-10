#version 430 compatibility

in vec3 fs_Normal;
in vec4 fs_Position;
in vec4 fs_LPosition;
in vec3 fs_WNormal;

out vec4 outColor;
uniform sampler2D u_ShadowMap;

uniform mat4 u_ViewInverse;
uniform vec3 u_Color;

uniform uvec2 resolution;

const uint MAX_LIGHTS_PER_TILE = 64;

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

layout (std430, binding=3) buffer tileLights
{
	uvec4 lightList[];
};

void main ()
{
	vec4 wPosition = u_ViewInverse * fs_Position;
	vec3 lColor = vec3 (1.0);
	vec3 outColor3 = vec3 (0.0, 0.0, 0.0);
	vec4 lightVec = vec4 (0.0);

	vec2 threadsPerBlock = vec2 (8.0, 8.0);
	vec2 res = vec2 (1.0/threadsPerBlock.x, 1.0/threadsPerBlock.y); 
	uint global_offset = uint (((floor (gl_FragCoord.y / res.y) * (float(resolution.x)/threadsPerBlock.x)) + floor (gl_FragCoord.x / res.x)) * MAX_LIGHTS_PER_TILE);
	
	uint numLightsThisTile = lightList [global_offset].z;
	for (uint i = 0; i < numLightsThisTile; ++i)
	{
		uvec4 current_light = lightList [global_offset+i];
		if (current_light.y == 1)
			lightVec = vec4(lights [current_light.x].xyz, 1.0) - wPosition;
		else
			lightVec = vec4(vpl [current_light.x].position.xyz, 1.0) - wPosition;
		float clampedDiffuseFactor = clamp (dot (fs_WNormal, normalize(lightVec.xyz)), 0.0, 1.0);
		outColor3 += (u_Color * clampedDiffuseFactor);		
	}

	outColor3 /= numLightsThisTile;
	outColor = vec4 (outColor3, 1.0);
}