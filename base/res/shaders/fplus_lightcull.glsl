#version 430 compatibility
#extension	GL_ARB_compute_shader:					enable
#extension	GL_ARB_shader_storage_buffer_object:	enable

struct	LightData
{
	vec4	position;
	vec4	intensity;
};

uniform sampler2D depthTex;

shared vec4 frustum [4];
shared float depthMin;
shared float depthMax;
shared uint depthMin_uint;
shared uint depthMax_uint;
shared uint atomicOffset;

uniform mat4 u_InvProj;
uniform mat4 u_View;
uniform int u_numLights;
uniform int u_numVPLs;

const uint MAX_LIGHTS_PER_TILE = 64;

layout (std430, binding=1) buffer lightInfo
{
	LightData lights[];
};

layout (std430, binding=2) buffer vplInfo
{
	LightData vpl[];
};

layout (std430, binding=3) buffer tileLights
{
	uvec4 lightList[];
};

float getDepth (in uint pixel_x, in uint pixel_y)
{
	vec2 texCoords;
	texCoords.x = float (pixel_x / (gl_WorkGroupSize.x * gl_NumWorkGroups.x));
	texCoords.y = float (pixel_y / (gl_WorkGroupSize.y * gl_NumWorkGroups.y));
	return texture (depthTex, texCoords).z;
}

vec4 makePlane (in vec4 v1, in vec4 v2, in vec4 v3)
{
	vec4 vector1 = v2-v1;
	vec4 vector2 = v3-v1;
	vec3 normal = cross (vector1.xyz, vector2.xyz);

	return vec4 (normal.xyz, dot(normal, v1.xyz));
}

void checkAndAppendLight (in uint lightIndex, in uint buffertype)
{
	const float epsilon = 0.001;
	vec4 viewSpaceLightPos = vec4 (0);
	if (buffertype == 1)
		viewSpaceLightPos = u_View * lights [lightIndex].position;
	else if (buffertype == 2)
		viewSpaceLightPos = u_View * vpl [lightIndex].position;

	bool isContained = true;
	
	uint global_offset = ((gl_WorkGroupID.y * gl_NumWorkGroups.x) + gl_WorkGroupID.x) * MAX_LIGHTS_PER_TILE;
	if ((viewSpaceLightPos.z >= (depthMin-epsilon)) && (viewSpaceLightPos.z <= (depthMax+epsilon)))
	{
		for (int i = 0; i < 4; ++i)
		{
			if (dot (frustum [i].xyz, viewSpaceLightPos.xyz) < -epsilon)
			{
				isContained = false;
				break;
			}
		}

		if (isContained)
		{
			uint local_offset = atomicAdd (atomicOffset, 1);
			if (local_offset < MAX_LIGHTS_PER_TILE)
				lightList [global_offset + local_offset] = uvec4 (lightIndex, buffertype, 0, 0);
		}
	}
	barrier ();

	if ((gl_LocalInvocationID.x == 0) && (gl_LocalInvocationID.y == 0))
		lightList [global_offset].z = atomicOffset;
}

vec4 projectionToViewSpace (in uint pixel_x, in uint pixel_y, in float depth)
{
	vec4 projPosition = vec4 (0.0);
	projPosition.x = float (pixel_x / (gl_WorkGroupSize.x * gl_NumWorkGroups.x));
	projPosition.y = float (pixel_y / (gl_WorkGroupSize.y * gl_NumWorkGroups.y));
	projPosition.z = depth;
	projPosition.w = 1.0;

	projPosition = u_InvProj * projPosition;
	projPosition = projPosition / projPosition.w;

	return projPosition;
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main ()
{
	if ((gl_LocalInvocationID.x == 0) && (gl_LocalInvocationID.y == 0))
	{
		vec4 vertices [4];
		vertices [0] = projectionToViewSpace (gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, 1.0);
		vertices [1] = projectionToViewSpace (gl_GlobalInvocationID.x+7, gl_GlobalInvocationID.y, 1.0);
		vertices [2] = projectionToViewSpace (gl_GlobalInvocationID.x+7, gl_GlobalInvocationID.y+7, 1.0);
		vertices [3] = projectionToViewSpace (gl_GlobalInvocationID.x, gl_GlobalInvocationID.y+7, 1.0);

		for (int i = 0; i < 4; ++ i)
			frustum [i] = makePlane (vec4(0.0), vertices[i], vertices[(i+1)&3]);

		atomicOffset = 0;
		depthMin_uint = 0;
		depthMax_uint = 0xffffffff;
	}
	barrier ();

	float depth = getDepth (gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
	vec4 viewSpacePosition = projectionToViewSpace (gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, depth);

	uint depth_uint = floatBitsToUint (viewSpacePosition.z);
	if (depth < 1.0)
	{
//		atomicMin (depthMin_uint, depth_uint);
//		atomicMax (depthMax_uint, depth_uint);
	}
	barrier ();

	if ((gl_LocalInvocationID.x == 0) && (gl_LocalInvocationID.y == 0))
	{
		depthMin = uintBitsToFloat (depthMin_uint);
		depthMax = uintBitsToFloat (depthMax_uint);
	}
	barrier ();

	uint numThreads = gl_WorkGroupSize.x * gl_WorkGroupSize.y;
	for (uint i = 0; i < u_numLights; i+=numThreads)
	{
		uint lightIndex = i + gl_LocalInvocationIndex;
		if (lightIndex < u_numLights)
			checkAndAppendLight (lightIndex, 1);
	}
	barrier ();

	for (uint i = 0; i < u_numVPLs; i+=numThreads)
	{
		uint lightIndex = i + gl_LocalInvocationIndex;
		if (lightIndex < u_numLights)
			checkAndAppendLight (lightIndex, 2);
	}
}
