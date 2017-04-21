#pragma once

/*
	Angelcode bitmap font class.
	TODO: text format loading? Support for distance fields. Support for packed & non-32bit fonts?

	version 1.5 - now loads the texture file from the same directory as the font data file
	version 1.4 - fixed character yoffset calculations for Blit3D coordinate system
	version 1.3 - fixed incorrect verts array index if glyph code is stored more than once in the font file
	version 1.2 - added kerning support
	version 1.1 - uses unordered_map instead of array to store characters, so supports Unicode sets with values > 255
	version 1.0 - supports 32-bit single-page color fonts in binary format. 
*/

#include <string>
#include <vector>
#include <stdint.h>

#include "Blit3D.h"
#include <unordered_map>

class Blit3D;

namespace B3D
{
	class TVertex;
}

class AngelcodeCharDescriptor
{
public:
	float x, y;
	float width, height;
	float xOffset, yOffset;
	float xAdvance;
	int lookupVerts;
	std::unordered_map<int32_t, float> kerningTable;

	AngelcodeCharDescriptor() : x(0), y(0), width(0), height(0), xOffset(0), yOffset(0),
		xAdvance(0), lookupVerts(0)
	{ }
};

class AngelcodeFont
{
private:
	char endian;
	float lineHeight;
	float base;
	float scaleW, scaleH;
	std::unordered_map<int32_t, AngelcodeCharDescriptor> Chars;

	//TODO: make verts local to the constructer instead of a member var?
	B3D::TVertex *verts;  // memory for vertice data
	GLuint vboId;	// ID of VBO
	GLuint vaoId;	//ID of the VAO 		

	GLuint texId; //ID of texture
	std::string textureName; //filename of the texture
	TextureManager *texManager; //pointer to the global texture manager
	glm::mat4 modelMatrix; // Store the model matrix 
	int modelMatrixLocation; // Store the location of our model matrix in the shader
	int alphaLocation; //store the location of the alpha variable in the shader
	GLSLProgram *prog; //our shader for 2d rendering
	
	int16_t ReadShort(int offset, char buffer[]);
	int32_t ReadInt(int offset, char buffer[]);
	int16_t ReadShortAndAdvance(int &offset, char buffer[]);
	int32_t ReadIntAndAdvance(int &offset, char buffer[]);
	uint32_t AngelcodeFont::ReadUIntAndAdvance(int &offset, char buffer[]);

public:
	GLfloat dest_x; //window coordinates of the center of the sprite, in pixels
	GLfloat dest_y;
	GLfloat angle; //angle of the sprite, in degrees
	GLfloat alpha;

	void BlitText(float x, float y, std::string output); //draws the string
	float WidthText(std::string output);//returns the width of the text string, in pixels
	~AngelcodeFont();
	AngelcodeFont(std::string fontfile, TextureManager *TexManager, GLSLProgram *shader);

};

std::string DirectoryOfFilePath(const std::string& filename);