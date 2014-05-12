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
mesh(meshPtr),
requiresUpdate(false)
{
    if (mesh != NULL)
    {
        glGenVertexArrays(1, &vertexArray);
        vertexBufferObjects.resize(3);
        glGenBuffers(3, &vertexBufferObjects[0]);
        glGenBuffers(1, &indexBuffer);

        glBindVertexArray(vertexArray);
        {
            glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjects[position]);
            glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray(position);

            glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjects[normal]);
            glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray(normal);

            glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjects[texcoord]);
            glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray(texcoord);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size() * sizeof(GLuint), &mesh->indices[0], GL_STATIC_DRAW);
    }
}

RenderObject::~RenderObject()
{
    glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(3, &vertexBufferObjects[0]);
    glDeleteBuffers(1, &indexBuffer);
}

void RenderObject::Update()
{
    if (mesh)
    {
        if (requiresUpdate)
        {
            glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjects[position]);
            float* verticesArrayPtr = static_cast<float*>(glMapBuffer(GL_ARRAY_BUFFER, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
            for (int i = 0; i < mesh->vertices.size(); i++)
            {
                verticesArrayPtr[3 * i] = mesh->vertices[i].x;
                verticesArrayPtr[3 * i] = mesh->vertices[i].y;
                verticesArrayPtr[3 * i] = mesh->vertices[i].z;
            }
            glUnmapBuffer(GL_ARRAY_BUFFER);

            glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjects[normal]);
            float* normalsArrayPtr = static_cast<float*>(glMapBuffer(GL_ARRAY_BUFFER, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
            for (int i = 0; i < mesh->normals.size(); i++)
            {
                verticesArrayPtr[3 * i] = mesh->normals[i].x;
                verticesArrayPtr[3 * i] = mesh->normals[i].y;
                verticesArrayPtr[3 * i] = mesh->normals[i].z;
            }
            glUnmapBuffer(GL_ARRAY_BUFFER);
            requiresUpdate = false;

            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
}

void RenderObject::Render()
{
    Update();
    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}