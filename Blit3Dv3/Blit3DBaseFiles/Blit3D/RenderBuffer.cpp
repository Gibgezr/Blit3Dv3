#include "RenderBuffer.h"

RenderBuffer::RenderBuffer(int width, int height, TextureManager *TexManager, std::string name, Blit3D *blit3d)
{
	b3d = blit3d;
	texwidth = width;
	texheight = height;
	texManager = TexManager;
	texname = name;
	// generate namespace for the frame buffer, colorbuffer and depthbuffer
	glGenFramebuffers(1, &fb);
	glGenTextures(1, &color_tex);
	glGenRenderbuffers(1, &depth_rb);
	//switch to our fbo so we can bind stuff to it
	glBindFramebuffer(GL_FRAMEBUFFER, fb);

	//create the colorbuffer texture and attach it to the frame buffer
	glBindTexture(GL_TEXTURE_2D, color_tex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //for when we are close

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
		GL_RGBA, GL_INT, NULL);

	glFramebufferTexture2D(GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, color_tex, 0);

	//ad the texture to the TM
	texManager->AddLoadedTexture(name, color_tex);

	// create a render buffer as our depth buffer and attach it
	glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
	glRenderbufferStorage(GL_RENDERBUFFER,
		GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER, depth_rb);

	//test our FBO
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	assert(status == GL_FRAMEBUFFER_COMPLETE);

	// Go back to regular frame buffer rendering
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//create our sprite
	sprite = b3d->MakeSprite(this);
}

RenderBuffer::~RenderBuffer()
{
	b3d->DeleteSprite(sprite);
	glDeleteFramebuffers(1, &fb);
	glDeleteRenderbuffers(1, &depth_rb);
	//the next line will bomb if the TM has already been freed,
	//so be sure to delete all RenderBuffers *before* deleting
	//the TM, just like Sprites
	texManager->FreeTexture(texname);
}

void RenderBuffer::RenderToMe(GLSLProgram *shader)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	b3d->ReshapFBO(texwidth, texheight, shader);
	//save shader program for when we are done and need to reset the perspective matrix
	prog = shader;
}

void RenderBuffer::RenderToMe()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	b3d->ReshapFBO(texwidth, texheight, b3d->shader2d);
	//save shader program for when we are done and need to reset the perspective matrix
	prog = b3d->shader2d;
}

void RenderBuffer::DoneRendering()
{
	b3d->Reshape(prog);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}