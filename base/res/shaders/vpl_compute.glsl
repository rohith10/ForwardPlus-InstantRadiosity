#version 430 compatibility
#extension	GL_ARB_compute_shader:					enable
#extension	GL_ARB_shader_storage_buffer_object:	enable

struct	LightData
{
	vec3	position;
	float	intensity;
};

struct	Ray
{
	vec3	origin;
	vec3	direction;
};

layout (std140, binding = 1) buffer lightPos
{
	struct LightData lights [];
};

layout (std140, binding = 2) buffer rayInfo
{
	struct Ray rays [];
};

uniform int u_numLights;
uniform int u_bounceNo;

vec3 randDirHemisphere (in vec3 normal, in float v1, in float v2);
float boxIntersectionTest (in vec3 boxMin, in vec3 boxMax, in Ray r, out vec3 intersectionPoint);

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main(void)
{
	
/*
	if (m.hasReflective >= 1.0) // specular reflectance
	{
		r.direction = glm::normalize (reflectRay (r.direction, normal));
		retVal = 1;
	}
	else if (m.hasRefractive)  // Fresnel refractance.
	{
		float cosIncidentAngle = glm::dot (r.direction, normal);
		float insideRefIndex = m.indexOfRefraction; float outsideRefIndex = 1.0;
		if (cosIncidentAngle > 0)	// If ray going from inside to outside.
		{
			outsideRefIndex = m.indexOfRefraction;
			insideRefIndex = 1.0;
			normal = -normal;
		}

		if (calculateFresnelReflectance (outsideRefIndex, insideRefIndex, cosIncidentAngle, u01(rng)))
		{	
//			if (cosIncidentAngle > 0)	// If ray going from inside to outside.
//				normal = -normal;		// Flip the normal for reflection.
			r.direction = glm::normalize (reflectRay (r.direction, normal));
			retVal = 1;
		}
		else
		{
			// As given in Real-Time Rendering, Third Edition, pp. 396.
			/*float w = (outsideRefIndex / insideRefIndex) * glm::dot (lightDir, normal);
			float k = sqrt (1 + ((w + (outsideRefIndex / insideRefIndex)) * (w - (outsideRefIndex / insideRefIndex))));
			r.direction = (w - k)*normal - (outsideRefIndex / insideRefIndex)*lightDir;*/
/*			r.direction = glm::normalize (glm::refract (r.direction, normal, outsideRefIndex/insideRefIndex));
			r.origin = intersect+0.01f*r.direction;
			retVal = 2;
		}
	}
	else if (m.hasReflective)	// m.hasReflective between 0 and 1 signifies diffuse reflectance.
	{
		r.direction = glm::normalize (calculateDirectionInLobeAroundNormal (normal, rng));
		retVal = 1;
	}
	else
	{
		float xi1, xi2;
		r.direction = normalize (randDirHemisphere (normal, u01 (rng), u02 (rng)));
	}*/
}

vec3 randDirHemisphere (vec3 normal, float v1, float v2) 
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
    
    vec3 basis1 = normalize (cross (normal, someDirNotNormal));
    vec3 basis2 = normalize (cross (normal, basis1));
    
    return (cosPhi * normal) + (sinPhi*cos (theta) * basis1) + (sinPhi*sin (theta) * basis2);    
}

float boxIntersectionTest (in vec3 boxMin, in vec3 boxMax, in Ray r, out vec3 intersectionPoint)
{
	// Uses the slab method to check for intersection.
	// Refer http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtinter3.htm for details.

	// Define the constants. tnear = -INFINITY ; tfar = +INFINITY (+/- 1e6 for practical purposes)
	float tnear = -1000.0, tfar = 1000.0;
	float epsilon = 0.001;

	// Body space extremities.
	float lowerLeftBack [3] = {boxMin.x, boxMin.y, boxMin.z};
	float upperRightFront [3] = {boxMax.x, boxMax.y, boxMax.z};

	float rayOrigArr [3];
	rayOrigArr [0] = r.origin.x;
	rayOrigArr [1] = r.origin.y;
	rayOrigArr [2] = r.origin.z;

	float rayDirArr [3];
	rayDirArr [0] = r.direction.x;
	rayDirArr [1] = r.direction.y;
	rayDirArr [2] = r.direction.z;

	// For each X, Y and Z, check for intersections using the slab method as described above.
	int loopVar;
	for (loopVar = 0; loopVar < 3; ++loopVar)
	{
		if (fabs (rayDirArr [loopVar]) < epsilon)
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
				return -1.0;

			if (tfar < 0-epsilon)
				return -1.0;
		}
	}

	intersectionPoint = r.origin + (r.direction * tnear);
	return tnear; //length (r.origin - intersectionPoint);
}