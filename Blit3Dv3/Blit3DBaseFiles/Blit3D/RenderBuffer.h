#pragma once
#include "Blit3D.h"

class Blit3D;
class Sprite;

class RenderBuffer
{
public:
	GLuint fb;//FBO
	GLuint color_tex; //texture ID
	GLuint depth_rb;//depth buffer
	TextureManager *texManager;
	std::string texname;
	int texwidth;
	int texheight;
	Blit3D *b3d;
	GLSLProgram *prog;
	Sprite *sprite; //so we can draw with this render buffer

	RenderBuffer(int width, int height, TextureManager *TexManager, std::string name, Blit3D *blit3d);
	~RenderBuffer();
	void RenderToMe(GLSLProgram *shader);
	void RenderToMe();
	void DoneRendering();
};