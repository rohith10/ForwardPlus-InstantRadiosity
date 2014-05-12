
#include	"Renderer.h"

ForwardRenderer::ForwardRenderer() 
	: Renderer(1), 
	  ShadowMapFBO(0), 
      constructed(false)
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

ForwardRenderer::~ForwardRenderer()
{
    if (constructed)
        glDeleteFramebuffers(1, &ShadowMapFBO);
}

void	ForwardRenderer::Render()
{
    //	glUseProgram (forward_shading_prog);
    PopulateLights();

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    /*glEnable (GL_DEPTH_TEST);
    glColorMask (GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    glDepthFunc (GL_LESS);
    glDepthMask (GL_TRUE);
    draw_mesh_forward ();

    glEnable (GL_DEPTH_TEST);
    glColorMask (GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT);
    glDepthFunc (GL_LEQUAL);
    glDepthMask (GL_FALSE);*/

    draw_mesh_forward();
    glDisable(GL_DEPTH_TEST);
}
