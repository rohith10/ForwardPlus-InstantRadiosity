#pragma once

#include "glm/glm.hpp"
#include <vector>

struct PolyMesh
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<GLuint>    indices;
    enum 
    {
        MATERIAL_OPAQUE,
        MATERIAL_TRANSPARENT
    }   materialType;   // Have a render list for each material.
    unsigned int materialID;
};