#pragma once

#include	<vector>
#include	<GL/glew.h>

enum { DEPTH, ALBEDO, NORMAL, TEXCOORD, POSITION, DEPTH_MAP };

class Renderer
{
protected:
	std::vector<GLuint> textures;
	unsigned int	maxTextures;

public:
	Renderer(unsigned int nTextures = 0) : textures(nTextures), maxTextures(nTextures)
	{
		for (int i = 0; i < nTextures; ++i)
			glGenTextures(1, &textures[i]);
	}
	virtual void Render() = 0;
	virtual ~Renderer()
	{
		for (int i = 0; i < maxTextures; ++i)
			glDeleteTextures(1, &textures[i]);
	}
};

class DeferredRenderer : public Renderer
{
	std::vector<GLuint> FBO;

	void setFBO(int FBOid)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, FBO[FBOid]);
	}

public:
	DeferredRenderer(unsigned int nTextures = 7, unsigned int nBuffers = 3) : Renderer(nTextures), FBO(0)
	{}
	virtual void Render();
	~DeferredRenderer()
	{
		for (unsigned int i = 0; i < FBO.size(); ++i)
			glDeleteFramebuffers(1, &FBO[i]);
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
	~ForwardRenderer()	
	{
		glDeleteFramebuffers(1, &ShadowMapFBO);
	}
};

class FPlusRenderer : public ForwardRenderer
{
public:
	FPlusRenderer()
	{}
	virtual void Render();
	~FPlusRenderer()	{}
};