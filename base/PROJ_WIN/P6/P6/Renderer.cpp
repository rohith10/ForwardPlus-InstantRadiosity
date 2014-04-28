#include "Renderer.h"
#include <iostream>

Renderer::Renderer(unsigned int nTextures = 0) 
: textures(nTextures), 
  maxTextures(0)
{
    for (int i = 0; i < nTextures; ++i)
        glGenTextures(1, &textures[i]);
    maxTextures = textures.size();
    drawLists.clear();
}

RenderObject::RenderObject(PolyMesh* meshPtr) :
vertexArray(0),
indexBuffer(0),
mesh(meshPtr)
{
    if (mesh != NULL)
    {
        glGenVertexArrays(1, &vertexArray);
        vertexBufferObjects.resize(2);
        glGenBuffers(2, &vertexBufferObjects[0]);
        glGenBuffers(1, &indexBuffer);

        glBindVertexArray(vertexArray);

    }
}

RenderObject::~RenderObject()
{
    if (mesh)
    {
        delete mesh;
    }

    glDeleteVertexArrays(1, &vertexArray);
}