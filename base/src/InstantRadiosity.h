#pragma once

#include	<glm/glm.hpp>

struct	LightData
{
	glm::vec3	position;
	float		intensity;
};

struct	Ray
{
	glm::vec3	origin;
	glm::vec3	direction;
};

glm::vec3 randDirHemisphere (glm::vec3 normal, float v1, float v2);
