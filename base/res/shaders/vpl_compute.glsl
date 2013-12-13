#version 430 compatibility
#extension	GL_ARB_compute_shader:					enable
#extension	GL_ARB_shader_storage_buffer_object:	enable

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

struct	LightData
{
	vec4	position;
	vec4	intensity;
};

struct	Ray
{
	vec4	origin;
	vec4	direction;
	vec4	intensity;
};

struct	bBox
{
	vec4	minPt;
	vec4	maxPt;
	vec4	material;
};

uniform int u_bounceNo;

uniform int u_numLights;
uniform int u_numBounces;
uniform int u_numGeometry;
uniform int u_numVPLs;

layout (std430, binding=1) buffer vplInfo
{
	LightData vpl[];
};

layout (std430, binding=2) buffer rayInfo
{
	Ray rays[];
};

layout (std430, binding=3) buffer bBoxInfo
{
	bBox bBoxes[];
};

vec3 randDirHemisphere (in vec4 normal, in float v1, in float v2) 
{    
    float cosPhi = sqrt (v1);		
    float sinPhi = sqrt (1.0 - v1);	
    float theta = v2 * 2.0 * 3.141592;
        
	vec3 someDirNotNormal;
    if ((normal.x < normal.y) && (normal.x < normal.z)) 
      someDirNotNormal = vec3 (1.0, 0.0, 0.0);
    else if (normal.y < normal.z)
      someDirNotNormal = vec3 (0.0, 1.0, 0.0);
    else
      someDirNotNormal = vec3 (0.0, 0.0, 1.0);
    
    vec3 basis1 = normalize (cross (normal.xyz, someDirNotNormal));
    vec3 basis2 = normalize (cross (normal.xyz, basis1));
    
    return (cosPhi * normal.xyz) + (sinPhi*cos (theta) * basis1) + (sinPhi*sin (theta) * basis2);    
}

float boxIntersectionTest (in vec3 boxMin, in vec3 boxMax, in vec4 origin, in vec4 direction, out vec4 intersectionPoint, out vec4 intrPtNormal)
{
	// Uses the Kay-Kajiya slab method to check for intersection of ray r with box specified by boxMin and boxMax.
	// Refer http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter3.htm for details.

	// Define the constants. tnear = -INFINITY ; tfar = +INFINITY (+/- 1e6 for practical purposes)
	float tnear = -1000.0, tfar = 1000.0;
	float epsilon = 0.001;

	// Extremities.
	float lowerLeftBack [3] = {boxMin.x, boxMin.y, boxMin.z};
	float upperRightFront [3] = {boxMax.x, boxMax.y, boxMax.z};

	float rayOrigArr [3] = {origin.x, origin.y, origin.z};
	//rayOrigArr [0] = r.origin.x;
	//rayOrigArr [1] = r.origin.y;
	//rayOrigArr [2] = r.origin.z;

	float rayDirArr [3] = {direction.x, direction.y, direction.z};
	//rayDirArr [0] = r.direction.x;
	//rayDirArr [1] = r.direction.y;
	//rayDirArr [2] = r.direction.z;

	// For each X, Y and Z, check for intersections using the slab method as described above.
	// TODO replace with swizzled code and see how that pans out.
	int loopVar;
	for (loopVar = 0; loopVar < 3; loopVar++)
	{
		if (abs (rayDirArr [loopVar]) < epsilon)
		{
			if ((rayOrigArr [loopVar] < lowerLeftBack [loopVar]-epsilon) || (rayOrigArr [loopVar] > upperRightFront [loopVar]+epsilon))
				return -1.0;
		}
		else
		{
			float t1 = (lowerLeftBack [loopVar] - rayOrigArr [loopVar]) / rayDirArr [loopVar];
			float t2 = (upperRightFront [loopVar] - rayOrigArr [loopVar]) / rayDirArr [loopVar];

			if (t1 > t2+epsilon)
			{
				t2 += t1;
				t1 = t2 - t1;
				t2 -= t1;
			}

			if (tnear < t1-epsilon)
				tnear = t1;

			if (tfar > t2-epsilon)
				tfar = t2;

			if (tnear > tfar+epsilon)
				return -2.0;

			if (tfar < -epsilon)
				return -3.0;
		}
	}

	if (tnear < -epsilon)
		tnear = tfar;

	intersectionPoint = origin + (direction * tnear);
	vec4 centrePoint = vec4(((boxMin + boxMax) / 2.0).xyz, 1.0);
	vec4 differenceVec = intersectionPoint - centrePoint; 
	intrPtNormal = differenceVec;		// Not axis aligned!

	float x_comp = dot (intrPtNormal, vec4 (1.0, 0.0, 0.0, 0.0));
	float y_comp = dot (intrPtNormal, vec4 (0.0, 1.0, 0.0, 0.0));	
	float z_comp = dot (intrPtNormal, vec4 (0.0, 0.0, 1.0, 0.0));

	intrPtNormal = vec4 (0.0);
	if ((x_comp > epsilon) && (abs (x_comp-differenceVec.x) < epsilon))
		intrPtNormal.x = x_comp;
	else if ((y_comp > epsilon) && (abs (y_comp-differenceVec.y) < epsilon))
		intrPtNormal.y = y_comp;
	else if ((z_comp > epsilon) && (abs (y_comp-differenceVec.z) < epsilon))
		intrPtNormal.z = z_comp;

	return abs(tnear); 
}

// Returns a float in the range [0, 1].
// Code from opengl-tutorial.com
// Source: http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/
float random (in vec4 seed)
{
	float dot_product = dot (seed, vec4(12.9898,78.233,45.164,94.673));
    return fract (sin (dot_product) * 43758.5453);
}

void main(void)
{
	uint index = gl_WorkGroupSize.x * gl_NumWorkGroups.x * gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;
	uint maxIndex = (u_numVPLs/u_numBounces)*u_numLights;
	int bBoxIndex = -1;
	if (index < maxIndex)
	{
		int loopVar = 0;
		float tmin = 100000.0;
		vec4 intr_point = vec4 (0), intr_point2, intr_normal, intr_norm2;
		float t = 100000.0;
		for (loopVar = 0; loopVar < u_numGeometry; ++loopVar)
		{
			t = boxIntersectionTest (bBoxes [loopVar].minPt.xyz, bBoxes [loopVar].maxPt.xyz, rays [index].origin, rays [index].direction, intr_point2, intr_norm2);
			if ((t > 0.0) && (t < tmin))
			{
				tmin = t;
				intr_point = intr_point2;
				intr_normal = intr_norm2;
				bBoxIndex = loopVar;
			}
		}

		vpl [maxIndex*u_bounceNo + index].position = intr_point;

		vec4 vpl_colour = vec4 (0.0);
		float newIntensity = 0.0;
		if (tmin == 100000.0)		// No intersection at all with scene objects
		{
			vpl_colour = vec4 (0.0);	
			newIntensity = 0.0;  // Set intensity to 0 to indicate that light doesn't exist.
		}
		else
		{
			if (rays [index].intensity.w > 0.00001)
			{
				newIntensity = rays [index].intensity.w / 16.0;//1.0/(float(u_numVPLs) / float(u_numBounces));
				vpl_colour = bBoxes [bBoxIndex].material; 
			}
		}
		vpl [maxIndex*u_bounceNo + index].intensity.xyz = vpl_colour.xyz;
		vpl [maxIndex*u_bounceNo + index].intensity.w = newIntensity;

		// TODO: Fragment shader colour adjustment - divide.

		vec4 random_input_1 = vec4 (gl_GlobalInvocationID.x / (gl_NumWorkGroups.x * gl_WorkGroupSize.x), 
									gl_GlobalInvocationID.y / (gl_NumWorkGroups.y * gl_WorkGroupSize.y),
									index / maxIndex, 0.5);
		vec4 random_input_2 = vec4 (gl_GlobalInvocationID.y / (gl_NumWorkGroups.y * gl_WorkGroupSize.y),
									gl_GlobalInvocationID.x / (gl_NumWorkGroups.x * gl_WorkGroupSize.x), 
									0.5, index / maxIndex);

		rays [index].origin = intr_point;
		rays [index].direction = normalize (vec4 (randDirHemisphere (intr_normal, random (random_input_1), random (random_input_2)).xyz, 0.0));
		rays [index].intensity.xyz = vpl_colour.xyz;
		rays [index].intensity.w = newIntensity;
	}
}