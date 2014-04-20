
#include	"Renderer.h"

ForwardRenderer::ForwardRenderer() 
	: Renderer(1), 
	ShadowMapFBO(0), constructed(false)
{
	glGenFramebuffers(1, &ShadowMapFBO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glBindFramebuffer(GL_FRAMEBUFFER, ShadowMapFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textures[0], 0);
	glDrawBuffer(GL_NONE);

	GLenum SMapFBOStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (SMapFBOStatus == GL_FRAMEBUFFER_COMPLETE)
		constructed = true;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void	ForwardRenderer::Render()
{
	;
}
