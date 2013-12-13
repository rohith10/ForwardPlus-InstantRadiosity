#version 430 compatibility
#extension	GL_ARB_compute_shader:					enable
#extension	GL_ARB_shader_storage_buffer_object:	enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

struct	LightData
{
	vec4	position;
	vec4	intensity;
};

uniform sampler2D depthTex;
layout (rgba32f) readonly uniform image2D depthTex2;

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

layout (std430, binding=1) buffer vplInfo
{
	LightData vpl[];
};

layout (std430, binding=2) buffer lightInfo
{
	vec4 lights[];
};

layout (std430, binding=3) buffer tileLights
{
	uvec4 lightList[];
};

layout (std430, binding=4) buffer debug
{
	vec4 dbgBuff[];
};

float getDepth (in uint pixel_x, in uint pixel_y)
{
	ivec2 texCoords = ivec2 (pixel_x, pixel_y);
//	texCoords.x = float (pixel_x) / float (gl_WorkGroupSize.x * gl_NumWorkGroups.x);
//	texCoords.y = float (pixel_y) / float (gl_WorkGroupSize.y * gl_NumWorkGroups.y);
	return imageLoad (depthTex2, texCoords).r;
}

vec4 makePlane (in vec4 v1, in vec4 v2, in vec4 v3)
{
	vec4 vector1 = v2-v1;
	vec4 vector2 = v3-v1;
	vec3 normal = cross (vector2.xyz, vector1.xyz);

	return vec4 (normal.xyz, dot(normal, v1.xyz));
}

void checkAndAppendLight (in uint lightIndex, in uint buffertype)
{
	const float epsilon = 0.001;
	vec4 viewSpaceLightPos = vec4 (0);
	uint global_offset = ((gl_WorkGroupID.y * gl_NumWorkGroups.x) + gl_WorkGroupID.x) * MAX_LIGHTS_PER_TILE;

	if (buffertype == 1)
	{	
//		uint local_offset = atomicAdd (atomicOffset, 1);
//		if (local_offset < MAX_LIGHTS_PER_TILE)
//			lightList [global_offset + local_offset] = uvec4 (lightIndex, buffertype, 0, 0);
		viewSpaceLightPos = u_View * lights [lightIndex];
	}
	else if (buffertype == 2)
	{	
		viewSpaceLightPos = u_View * vpl [lightIndex].position;
	}	
		if ((viewSpaceLightPos.x >= (frustum [0].x-epsilon)) && (viewSpaceLightPos.x <= (frustum [1].x+epsilon)))
		{
			if ((viewSpaceLightPos.y >= (frustum [0].y-epsilon)) && (viewSpaceLightPos.y <= (frustum [2].y+epsilon)))
			{
				uint local_offset = atomicAdd (atomicOffset, 1);
				if (local_offset < MAX_LIGHTS_PER_TILE)
					lightList [global_offset + local_offset] = uvec4 (lightIndex, buffertype, 0, 0);
			}
		}
//	}
	barrier ();
	memoryBarrierShared ();

	if ((gl_LocalInvocationID.x == 0) && (gl_LocalInvocationID.y == 0))
	{
		if (atomicOffset > MAX_LIGHTS_PER_TILE)
			atomicOffset = MAX_LIGHTS_PER_TILE;
		lightList [global_offset].z = atomicOffset;//(atomicOffset<MAX_LIGHTS_PER_TILE)? atomicOffset : MAX_LIGHTS_PER_TILE;
	}
}

vec4 projectionToViewSpace (in uint pixel_x, in uint pixel_y, in float depth)
{
	vec4 projPosition = vec4 (0.0);
	projPosition.x = float (pixel_x) / float(gl_WorkGroupSize.x * gl_NumWorkGroups.x);
	projPosition.y = float (pixel_y) / float(gl_WorkGroupSize.y * gl_NumWorkGroups.y);
	projPosition.z = depth;
	projPosition.w = 1.0;

	projPosition = u_InvProj * projPosition;
	projPosition = projPosition / projPosition.w;

	return projPosition;
}

void main ()
{
	uint global_offset = ((gl_WorkGroupID.y * gl_NumWorkGroups.x) + gl_WorkGroupID.x)*64;
	if ((gl_LocalInvocationID.x == 0) && (gl_LocalInvocationID.y == 0))
	{
		vec4 vertices [4];
		vertices [0] = projectionToViewSpace (gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, 1.0);
		vertices [1] = projectionToViewSpace (gl_GlobalInvocationID.x+8, gl_GlobalInvocationID.y, 1.0);
		vertices [2] = projectionToViewSpace (gl_GlobalInvocationID.x+8, gl_GlobalInvocationID.y+8, 1.0);
		vertices [3] = projectionToViewSpace (gl_GlobalInvocationID.x, gl_GlobalInvocationID.y+8, 1.0);

		for (int i = 0; i < 4; ++ i)
		{	
			frustum [i] = vertices [i];//makePlane (vec4(0.0), vertices[i], vertices[(i+1)&3]);
		}

		atomicOffset = 0;
		depthMax_uint = 0;
		depthMin_uint = 0xffffffff;
	}
	barrier ();
	memoryBarrierShared ();

	//float depth = getDepth (gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
	//vec4 viewSpacePosition = projectionToViewSpace (gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, depth);

	//uint depth_uint = floatBitsToUint (viewSpacePosition.z);
	//if (depth < 1.0)
	//{
	//	atomicMin (depthMin_uint, depth_uint);
	//	atomicMax (depthMax_uint, depth_uint);
	//}
	//barrier ();
	//memoryBarrierShared ();

	//if ((gl_LocalInvocationID.x == 0) && (gl_LocalInvocationID.y == 0))
	//{
	//	depthMin = uintBitsToFloat (depthMin_uint);
	//	depthMax = uintBitsToFloat (depthMax_uint);

	//	// Debug code:
	//	dbgBuff [global_offset] = vec4(depthMin);
	//	dbgBuff [global_offset+1] = vec4(depth);
	//}
	//barrier ();
	//memoryBarrierShared ();

	uint numThreads = gl_WorkGroupSize.x * gl_WorkGroupSize.y;
	for (uint i = 0; i < u_numLights; i+=numThreads)
	{
		uint lightIndex = i + gl_LocalInvocationIndex;
		if (lightIndex < u_numLights)
			checkAndAppendLight (lightIndex, 1);
	}
	barrier ();
	memoryBarrierShared ();

	for (uint i = 0; i < u_numVPLs; i+=numThreads)
	{
		uint lightIndex = i + gl_LocalInvocationIndex;
		if (lightIndex < u_numVPLs)
			if (length (vpl [lightIndex].intensity.xyz) > 0.0)
				checkAndAppendLight (lightIndex, 2);
	}
}
