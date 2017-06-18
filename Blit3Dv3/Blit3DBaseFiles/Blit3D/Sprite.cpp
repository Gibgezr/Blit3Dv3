#include "Sprite.h"

extern logger oLog;

//textured Sprite class --------------------------------------------------------------
Sprite::Sprite(GLfloat startX, GLfloat startY, GLfloat width, GLfloat height,
	std::string TextureFileName, TextureManager *TexManager, GLSLProgram *shader)
{
	dest_x = 0.f;
	dest_y = 0.f;
	angle = 0.f;
	alpha = 1.f;
	scale_x = scale_y = 1.f;

	GLfloat halfSizeX, halfSizeY;//x,y half-dimensions of the quad

	halfSizeX = width / 2.f;
	halfSizeY = height / 2.f;

	prog = shader;

	GLfloat imagewidth, imageheight;
	textureName = TextureFileName;
	texManager = TexManager;

	//load the texture via the texture manager
	texId = texManager->LoadTexture(TextureFileName);
	if(texId == 0)
	{
		oLog(Level::Severe) << "Image loading error while loading image file: " << TextureFileName << "for Sprite";
		assert(texId != 0);
	}

	texManager->FetchDimensions(TextureFileName, imagewidth, imageheight);

	GLfloat u1 = startX / imagewidth;
	GLfloat u2 = (startX + width) / imagewidth;

	GLfloat v1 = 1.f - (startY / imageheight);
	GLfloat v2 = 1.f - ((startY + height) / imageheight);


	verts = new B3D::TVertex[4]; //make an array of Textured Vertices

	// generate a new VAO and get the associated ID
	glGenVertexArrays(1, &vaoId); // Create our Vertex Array Object  
	glBindVertexArray(vaoId); // Bind our Vertex Array Object so we can use it  

	// generate a new VBO and get the associated ID
	glGenBuffers(1, &vboId);

	// bind VBO in order to use
	glBindBuffer(GL_ARRAY_BUFFER, vboId);

	//set the vertex array points...we need 4 vertices, one for each corner of our sprite, 

	/*

	0-------2
	|       |
	|       |
	|       |
	1-------3
	*/

	//front side, counterclockwise
	//point 0
	verts[0].x = -halfSizeX;				verts[0].y = halfSizeY;			verts[0].z = 0.f;
	verts[0].u = u1;	verts[0].v = v1;
	//point 1
	verts[1].x = -halfSizeX;				verts[1].y = -halfSizeY;		verts[1].z = 0.f;
	verts[1].u = u1;	verts[1].v = v2;
	//point 2
	verts[2].x = halfSizeX;					verts[2].y = halfSizeY;			verts[2].z = 0.f;
	verts[2].u = u2;	verts[2].v = v1;
	//point 3
	verts[3].x = halfSizeX;					verts[3].y = -halfSizeY;		verts[3].z = 0.f;
	verts[3].u = u2;	verts[3].v = v2;
	

	// upload data to VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(B3D::TVertex) * 4, verts, GL_STATIC_DRAW);

	// Set up our vertex attributes pointers
	///we don't really need normals for 2D, except maybe for special effects.
	//We send them anyway, as we don't have special shaders for 2d mode (YET!)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(B3D::TVertex), BUFFER_OFFSET(0)); //3 values (x,y,z) per point, start at 0 offset 	
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(B3D::TVertex), BUFFER_OFFSET(sizeof(GLfloat)* 3)); //Start after x,y,z data 


	// activate attribute array
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2); //don't use channel 2
	glDisableVertexAttribArray(3); //don't use Color channel, we are textured


	glBindVertexArray(0); // Disable our Vertex Array Object? 
	glBindBuffer(GL_ARRAY_BUFFER, 0);// Disable our Vertex Buffer Object

	//free the memory once it's been uploaded
	delete[] verts;
}

Sprite::Sprite(RenderBuffer * rb, TextureManager *TexManager, GLSLProgram *shader)
{
	dest_x = 0.f;
	dest_y = 0.f;
	angle = 0.f;
	alpha = 1.f;
	scale_x = scale_y = 1.f;

	GLfloat halfSizeX, halfSizeY;//x,y half-dimensions of the quad

	halfSizeX = rb->texwidth / 2.f;
	halfSizeY = rb->texheight / 2.f;

	prog = shader;

	GLfloat u1 = 0.f;
	GLfloat u2 = 1.f;

	GLfloat v1 = 1.f;
	GLfloat v2 = 0.f;

	textureName = rb->texname;
	texManager = TexManager;

	//get the texture from the renderBuffer
	texId = rb->color_tex;

	//increment our use of this texture
	texManager->AddLoadedTexture(textureName, texId);

	verts = new B3D::TVertex[4]; //make an array of Textured Vertices

	// generate a new VAO and get the associated ID
	glGenVertexArrays(1, &vaoId); // Create our Vertex Array Object  
	glBindVertexArray(vaoId); // Bind our Vertex Array Object so we can use it  

	// generate a new VBO and get the associated ID
	glGenBuffers(1, &vboId);

	// bind VBO in order to use
	glBindBuffer(GL_ARRAY_BUFFER, vboId);

	//set the vertex array points...we need 4 vertices, one for each corner of our sprite, 

	/*

	0-------2
	|       |
	|       |
	|       |
	1-------3
	*/

	//front side, counterclockwise
	//point 0
	verts[0].x = -halfSizeX;				verts[0].y = halfSizeY;			verts[0].z = 0.f;
	verts[0].u = u1;	verts[0].v = v1;
	//point 1
	verts[1].x = -halfSizeX;				verts[1].y = -halfSizeY;		verts[1].z = 0.f;
	verts[1].u = u1;	verts[1].v = v2;
	//point 2
	verts[2].x = halfSizeX;					verts[2].y = halfSizeY;			verts[2].z = 0.f;
	verts[2].u = u2;	verts[2].v = v1;
	//point 3
	verts[3].x = halfSizeX;					verts[3].y = -halfSizeY;		verts[3].z = 0.f;
	verts[3].u = u2;	verts[3].v = v2;

	// upload data to VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(B3D::TVertex) * 4, verts, GL_STATIC_DRAW);

	// Set up our vertex attributes pointers
	///we don't really need normals for 2D, except maybe for special effects.
	//We send them anyway, as we don't have special shaders for 2d mode (YET!)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(B3D::TVertex), BUFFER_OFFSET(0)); //3 values (x,y,z) per point, start at 0 offset 	
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(B3D::TVertex), BUFFER_OFFSET(sizeof(GLfloat)* 3)); //Start after x,y,z data 


	// activate attribute array
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2); //don't use channel 2
	glDisableVertexAttribArray(3); //don't use Color channel, we are textured

	glBindVertexArray(0); // Disable our Vertex Array Object? 
	glBindBuffer(GL_ARRAY_BUFFER, 0);// Disable our Vertex Buffer Object

	//free the memory once it's been uploaded
	delete[] verts;
}

Sprite::~Sprite()
{
	// free texture
	texManager->FreeTexture(textureName);

	// delete VBO when object destroyed
	glDeleteBuffers(1, &vboId);
	glDeleteVertexArrays(1, &vaoId);
}

void Sprite::Blit(void)
{
	glBindVertexArray(vaoId); // Bind our Vertex Array Object 

	//bind our texture
	texManager->BindTexture(texId);

	// set the rotation/translation matrix
	/*OpenGL has a special rule to draw fragments at the center of pixel screens,
	called "diamond rule" [2] [3]. Consequently, it is recommended to add a small translation
	in X,Y before drawing 2D sprite:
	glm::translate(glm::mat4(1), glm::vec3(0.375, 0.375, 0.));*/
	modelMatrix = glm::translate(glm::mat4(1.f), glm::vec3(dest_x, dest_y, 0.f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(angle), glm::vec3(0.f, 0.f, 1.f));

	//send our modelMatrix to the shader
	prog->setUniform("modelMatrix", modelMatrix);

	//send our alpha to the shader
	prog->setUniform("in_Alpha", alpha);
	//send the scaling
	prog->setUniform("in_Scale_X", scale_x);
	prog->setUniform("in_Scale_Y", scale_y);

	// draw a triangle strip
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// bind with 0, so, switch back to normal pointer operation
	glBindVertexArray(0);

	//reset scaling and alpha
	alpha = scale_x = scale_y = 1.f;
}

void Sprite::Blit(float x, float y)
{
	dest_x = x;
	dest_y = y;

	Blit();
}

void Sprite::Blit(float alpha_val)
{
	alpha = alpha_val;
	Blit();
}

void Sprite::Blit(float x, float y, float scale_val_x, float scale_val_y)
{
	scale_x = scale_val_x;
	scale_y = scale_val_y;
	dest_x = x;
	dest_y = y;

	Blit();
}


void Sprite::Blit(float x, float y, float scale_val_x, float scale_val_y, float alpha_val)
{
	scale_x = scale_val_x;
	scale_y = scale_val_y;
	alpha = alpha_val;
	dest_x = x;
	dest_y = y;

	Blit();
}