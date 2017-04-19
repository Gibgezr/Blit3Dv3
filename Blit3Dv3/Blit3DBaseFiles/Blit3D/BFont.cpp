#include "BFont.h"

extern logger oLog;

BFont::BFont(std::string TextureFileName, std::string widths_file, float fontsize, TextureManager *TexManager, GLSLProgram *shader)
{
	//load the texture via the texture manager
	texManager = TexManager;
	texId = texManager->LoadTexture(TextureFileName);
	if(texId == 0)
	{
		oLog(Level::Severe) << "Free Image loading error while loading image file: " << TextureFileName << "for Bfont";
		assert(texId != 0);
	}
	fontSize = fontsize;
	angle = 0.f;
	alpha = 1.f;	//-Fréderic Duguay

	textureName = TextureFileName;

	prog = shader;

	//load the widths data file
	std::ifstream data_file;
	data_file.open(widths_file.c_str(), std::ios::in | std::ios::binary);
	if(!data_file.is_open())
	{
		oLog(Level::Severe) << "Error while loading widths data file: " << widths_file << "for Bfont";
		assert(data_file.is_open());
	}

	short buffer[256]; //memory to read into
	char *buff = (char *)buffer;

	data_file.seekg(0, std::ios::beg);//seek to start

	//load entire file in one go...much faster than reading 2 chars at a time!
	data_file.read(buff, 512);
	data_file.close();

	for(int i = 0; i < 256; ++i)
	{
		widths[i] = (_int32)buffer[i];
	}


	verts = new B3D::TVertex[4 * 256]; //make an array of Textured Vertices

	// generate a new VAO and get the associated ID
	glGenVertexArrays(1, &vaoId); // Create our Vertex Array Object  
	glBindVertexArray(vaoId); // Bind our Vertex Array Object so we can use it  

	// generate a new VBO and get the associated ID
	glGenBuffers(1, &vboId);

	// bind VBO in order to use
	glBindBuffer(GL_ARRAY_BUFFER, vboId);

	//set the vertex array points...we need 4 vertices, one for each corner of our sprite,
	//per letter
	float	cx;										// Holds Our X Character Coord
	float	cy;										// Holds Our Y Character Coord
	int		loop;

	for(loop = 0; loop<256; loop++)
	{
		cx = ((float)(loop % 16)) / 16.0f;				// X Position Of Current Character
		cy = ((float)(loop / 16)) / 16.0f;				// Y Position Of Current Character

		verts[loop * 4].x = 0; verts[loop * 4].y = 0;		// Vertex Coord (Bottom Left)
		verts[loop * 4].u = cx;	verts[loop * 4].v = 1 - (cy + 1.f / 16);	// Texture Coord (Bottom Left)


		verts[loop * 4 + 1].x = fontsize;	verts[loop * 4 + 1].y = 0;	// Vertex Coord (Bottom Right)	
		verts[loop * 4 + 1].u = cx + (1.f / 16);	verts[loop * 4 + 1].v = 1 - (cy + 1.f / 16);	// Texture Coord (Bottom Right)

		verts[loop * 4 + 2].x = fontsize;	verts[loop * 4 + 2].y = fontsize;	// Vertex Coord (Top Right)
		verts[loop * 4 + 2].u = cx + (1.f / 16);	verts[loop * 4 + 2].v = 1 - cy;	// Texture Coord (Top Right)


		verts[loop * 4 + 3].x = 0;	verts[loop * 4 + 3].y = fontsize;		// Vertex Coord (Top Left)							
		verts[loop * 4 + 3].u = cx;	verts[loop * 4 + 3].v = 1 - cy;	// Texture Coord (Top Left)


		verts[loop * 4].z = verts[loop * 4 + 1].z = verts[loop * 4 + 2].z = verts[loop * 4 + 3].z = 0.f;
	}

	// upload data to VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(B3D::TVertex) * 4 * 256, verts, GL_STATIC_DRAW);

	// Set up our vertex attributes pointers
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(B3D::TVertex), BUFFER_OFFSET(0)); //3 values (x,y,z) per point, start at 0 offset 
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(B3D::TVertex), BUFFER_OFFSET(sizeof(GLfloat)* 3)); //Start after x,y,z, data 



	// activate attribute array
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2); // don'yt use channel 2
	glDisableVertexAttribArray(3); //don't use Color channel, we are textured

	glBindVertexArray(0); // Disable our Vertex Array Object? 
	glBindBuffer(GL_ARRAY_BUFFER, 0);// Disable our Vertex Buffer Object

	//find the modelMatrix location in the current shader
	//modelMatrixLocation = glGetUniformLocation(shader->id(), "modelMatrix");
	//alphaLocation = glGetUniformLocation(shader->id(), "in_Alpha"); //-Fréderic Duguay

	//free the memory once it's been uploaded
	delete[] verts;
}

void BFont::BlitText(bool whichFont, float x, float y, std::string output)
{
	dest_x = x;
	dest_y = y;

	glBindVertexArray(vaoId); // Bind our Vertex Array Object 

	//bind our texture
	texManager->BindTexture(texId);

	// set the rotation/translation matrix
	modelMatrix = glm::translate(glm::mat4(1.f), glm::vec3(dest_x, dest_y, 0.f));
	modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.f, 0.f, 1.f));

	//send our alpha to the shader
	prog->setUniform("in_Alpha", alpha);

	//send our modelMatrix to the shader
	prog->setUniform("modelMatrix", modelMatrix);
	prog->setUniform("in_Scale_X", 1.f); //default scaling
	prog->setUniform("in_Scale_Y", 1.f); //default scaling
	int letter;

	float scale = fontSize / 128;
	for(unsigned int i = 0; i < output.size(); ++i)
	{
		letter = output[i] - 32;
		if(whichFont) letter += 128;
		// draw a quad: 1 quad x 4points per quad = 4 verts, the third argument
		glDrawArrays(GL_QUADS, letter * 4, 4);
		modelMatrix = glm::translate(modelMatrix, glm::vec3((float)widths[letter] * scale, 0.f, 0.f));
		prog->setUniform("modelMatrix", modelMatrix);
	}

	// bind with 0, so, switch back to normal pointer operation
	glBindVertexArray(0);

	return;
}

float BFont::WidthText(bool whichFont, std::string output)
{
	int letter;
	float width_text = 0;

	float scale = fontSize / 128;
	for(unsigned int i = 0; i < output.size(); ++i)
	{
		letter = output[i] - 32;

		if(whichFont) letter += 128;

		width_text += widths[letter] * scale;
	}

	return width_text;
}

BFont::~BFont(void)
{
	// free texture
	texManager->FreeTexture(textureName);

	// delete VBO when object destroyed
	glDeleteBuffers(1, &vboId);
	glDeleteVertexArrays(1, &vaoId);
}