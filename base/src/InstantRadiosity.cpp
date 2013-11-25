
#include	"InstantRadiosity.h"

// Gives a random direction over the hemisphere above the surface with the normal "normal".
// v1 and v2 are to be in the interval [0,1].
// If normal is of length 0, then this gives a random direction in a sphere centered around the point.
glm::vec3 randDirHemisphere (glm::vec3 normal, float v1, float v2) 
{    
    float cosPhi = sqrt (v1);		
    float sinPhi = sqrt (1.0 - v1);	
    float theta = v2 * 2.0 * 3.141592;

	if (glm::dot (normal, normal) < 0.0001f)
		return glm::vec3 (sinPhi*cos(theta), cosPhi, sinPhi*sin(theta));
        
	glm::vec3 someDirNotNormal;
    if ((normal.x < normal.y) && (normal.x < normal.z)) 
      someDirNotNormal = glm::vec3 (1.0, 0.0, 0.0);
    else if (normal.y < normal.z)
      someDirNotNormal = glm::vec3 (0.0, 1.0, 0.0);
    else
      someDirNotNormal = glm::vec3 (0.0, 0.0, 1.0);
    
    glm::vec3 basis1 = glm::normalize (glm::cross (normal, someDirNotNormal));
    glm::vec3 basis2 = glm::normalize (glm::cross (normal, basis1));
    
    return (cosPhi * normal) + (sinPhi*cos (theta) * basis1) + (sinPhi*sin (theta) * basis2);    
}