#version 430 compatibility

layout (pixel_center_integer) in vec4 gl_FragCoord;

in vec3 fs_Normal;
in vec4 fs_Position;
in vec4 fs_LPosition;
in vec3 fs_WNormal;

out vec4 outColor;
uniform sampler2D u_ShadowMap;

uniform mat4 u_ViewInverse;
uniform vec3 u_Color;

uniform uvec2 resolution;
uniform sampler2D depthTex;

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
	uint global_offset = uint (((floor (gl_FragCoord.y / threadsPerBlock.y)*(float(resolution.x)/threadsPerBlock.x)) + floor (gl_FragCoord.x / threadsPerBlock.x))) * MAX_LIGHTS_PER_TILE;
	
	uint numLightsThisTile = lightList [global_offset].z;
	uint local_offset = uint ((int (gl_FragCoord.y) % int (threadsPerBlock.y)) * int (threadsPerBlock.x)) + (int (gl_FragCoord.x) % int (threadsPerBlock.x));
	uint index = global_offset + local_offset;
	//if (index < (1280*720))
	//	dbgBuff [index] = vec4 (lightList [global_offset].z, gl_FragCoord.x, gl_FragCoord.y, global_offset);
	//memoryBarrierBuffer ();
	//if (lightList [global_offset].z > 0)
	//	outColor3 = vec3 (1.0, 0.0, 0.0);
	for (uint i = 0; i < numLightsThisTile; ++i)
	{
		if (numLightsThisTile == MAX_LIGHTS_PER_TILE)
			break;
		uvec4 current_light = lightList [global_offset+i];
		if (current_light.y == 1)
			lightVec = vec4(lights [current_light.x].xyz, 1.0) - wPosition;
		else
			lightVec = vec4(vpl [current_light.x].position.xyz, 1.0) - wPosition;
		float clampedDiffuseFactor = clamp (dot (fs_WNormal, normalize(lightVec.xyz)), 0.0, 1.0);
		outColor3 += (u_Color * clampedDiffuseFactor);
	}

	if (numLightsThisTile > 0)
		outColor3 /= numLightsThisTile;
	//vec2 texCoord = gl_FragCoord.xy;
	//texCoord -= vec2 (0.5);
	//texCoord /= vec2(float(resolution.x)-1.0, float(resolution.y)-1.0);
	//outColor3 = vec3(texture (depthTex, texCoord).z);
	outColor = vec4 (/*vec3(numLightsThisTile)*/outColor3, 1.0);
}