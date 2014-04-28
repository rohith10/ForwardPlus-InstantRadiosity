#pragma once

#include	<vector>
#include    <list>
#include	<GL/glew.h>
#include    "PolyMesh.h"

class RenderObject;

class Renderer
{
protected:
	std::vector<GLuint> textures;
    std::vector<std::list<RenderObject>> drawLists;
	unsigned int	maxTextures;

public:
    Renderer(unsigned int nTextures = 0);
	virtual void Render() = 0;
	virtual ~Renderer()
	{
		for (int i = 0; i < maxTextures; ++i)
			glDeleteTextures(1, &textures[i]);
	}
};

class DeferredRenderer : public Renderer
{
	std::vector<GLuint> FBOlist;
    enum { DEPTH, ALBEDO, NORMAL, TEXCOORD, POSITION, DEPTH_MAP };

	void setFBO(int FBOid)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, FBOlist[FBOid]);
	}

public:
	DeferredRenderer(unsigned int nTextures = 7, unsigned int nBuffers = 3) : Renderer(nTextures), FBOlist(0)
	{}
	virtual void Render();
	~DeferredRenderer()
	{
		for (unsigned int i = 0; i < FBOlist.size(); ++i)
			glDeleteFramebuffers(1, &FBOlist[i]);
	}
};

class ForwardRenderer : public Renderer
{
protected:
	GLuint	ShadowMapFBO;
	bool	constructed;
public:
	ForwardRenderer();
	virtual void Render();
    virtual ~ForwardRenderer();
};

class FPlusRenderer : public ForwardRenderer
{
public:
	FPlusRenderer() {}
	virtual void Render();
	~FPlusRenderer() {}
};

class RenderObject
{
    GLuint vertexArray;
    std::vector<GLuint> vertexBufferObjects;
    GLuint indexBuffer;
    PolyMesh* mesh;

public:
    RenderObject(PolyMesh* meshPtr);
    ~RenderObject();
};