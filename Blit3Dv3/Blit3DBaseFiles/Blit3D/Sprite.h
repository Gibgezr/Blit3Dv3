#pragma once
#include "Blit3D.h"

class Blit3D;
class RenderBuffer;

namespace B3D
{
	class TVertex;
	class JoystickState;
}

class Sprite
{
private:
	B3D::TVertex *verts;  // memory for vertice data
	GLuint vboId;	// ID of VBO
	GLuint vaoId;	//ID of the VAO 		

	GLuint texId; //ID of texture
	std::string textureName; //filename of the texture
	TextureManager *texManager; //pointer to the global texture manager
	glm::mat4 modelMatrix; // Store the model matrix 
	int modelMatrixLocation; // Store the location of our model matrix in the shader
	int alphaLocation; //store the location of the alpha variable in the shader

	GLSLProgram *prog; //shader program for 2D

public:
	GLfloat dest_x; //window coordinates of the center of the sprite, in pixels
	GLfloat dest_y;
	GLfloat angle; //angle of the sprite, in degrees
	GLfloat alpha; //amount of extra alpha-blending to apply, modifies opacity of the sprite
	GLfloat scale_x, scale_y; //scaling value, 1 = 100%, 0.5 = half size, etc.
	void Blit(void); //draw the sprite
	void Blit(float x, float y); //draw the sprite centered at x,y
	void Blit(float alpha_val); //draw the sprite with set alpha
	void Blit(float x, float y, float scale_val_x, float scale_val_y); //draw the sprite centered at x,y with set scale
	void Blit(float x, float y, float scale_val_x, float scale_val_y, float alpha_val); //draw the sprite centered at x,y with set scale and alpha

	//we won't call this constructor directly, we'll let the Blit3D object do that
	Sprite(GLfloat startX, GLfloat startY, GLfloat width, GLfloat height,
		std::string TextureFileName, TextureManager *TexManager, GLSLProgram *shader);
	Sprite(RenderBuffer * rb, TextureManager *TexManager, GLSLProgram *shader);
	~Sprite();
};
