#pragma once

#include	<glm/glm.hpp>

struct	LightData
{
	glm::vec4	position;
	glm::vec4	intensity;
};

struct	Ray
{
	glm::vec4	origin;
	glm::vec4	direction;
	glm::vec4	intensity;
};

glm::vec3 randDirHemisphere (glm::vec3 normal, float v1, float v2);
