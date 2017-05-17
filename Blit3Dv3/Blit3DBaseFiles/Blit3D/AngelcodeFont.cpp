#include "AngelcodeFont.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include "ByteSwap.h"

extern logger oLog;

//helper function to find the path to a file
std::string DirectoryOfFilePath(const std::string& filename)
{
	size_t position = filename.find_last_of("\\/");
	
	if (position == std::string::npos) return "";
	else return filename.substr(0, position) + "\\";
}

AngelcodeFont::AngelcodeFont(std::string fontfile, TextureManager *TexManager, GLSLProgram *shader)
{
	texManager = TexManager;
	angle = 0.f;
	alpha = 1.f;
	prog = shader;

	//determine endianness of architecture
	unsigned char word[4] = { (unsigned char)0x01, (unsigned char)0x23, (unsigned char)0x45, (unsigned char)0x67 };

	unsigned long be = 0x01234567;
	unsigned long le = 0x67452301;
	unsigned long me = 0x23016745;
	unsigned long we;

	memcpy(&we, word, 4);
	endian = 0;
	if(we == le) endian = 0;
	else if(we == be) endian = 1;
	else if(we == me) endian = 2;
	
	//Reading a binary file in

	std::ifstream ifs;
	ifs.open(fontfile, std::ios::in | std::ios::binary| std::ios::ate);
		
	if(!ifs.is_open()) 
	{
		oLog(Level::Severe) << "Error while loading font data file: " << fontfile << "for AngelcodeFont";
		assert(ifs.is_open());
	}

	std::streampos end = ifs.tellg();
	ifs.seekg(0, std::ios::beg);
	std::streampos start = ifs.tellg();
	uint32_t fsize = (uint32_t) (end - start);

	int16_t *buf = new int16_t[fsize]; //memory to read into
	char *buffer = (char *)buf;

	//load entire file in one go...much faster than reading 2 chars at a time!
	ifs.read(buffer, fsize);

	ifs.close();

	//process buffer
	if(!(	buffer[0] == 'B'
		&&	buffer[1] == 'M'
		&&	buffer[2] == 'F'))
	{
		assert("Not a binary Angelcode font file" && 0);
	}

	if(buffer[3] != 3)
	{
		assert("Not a version 3 font file" && 0);
	}
		
	uint32_t current = 3;
	int vertsCounter = 0;
	while(current < fsize - 1)
	{
		current++;
		switch(buffer[current])
		{
		case 1://info block
		{
			current++; //advance to size data
			//move current to end of block
			int count = ReadInt(current, buffer);
			current += count + 3;
		}
			break;
		case 2: //common block
		{
			current++; //advance to size data
			//use marker to advance through this block
			int marker = current + 4; //move it past the block size data

			//move current to end of block
			int count = ReadInt(current, buffer);
			current += count + 3;
			
			lineHeight = (float)ReadShortAndAdvance(marker, buffer);			
			base = (float)ReadShortAndAdvance(marker, buffer);
			scaleW = (float)ReadShortAndAdvance(marker, buffer);
			scaleH = (float)ReadShortAndAdvance(marker, buffer);
			
			if(ReadShortAndAdvance(marker, buffer) != 1)
			{
				assert("More than one texture page" && 0);
			}
		}
			break;

		case 3: //page block
		{
			current++; //advance to size data
			//use marker to advance through this block
			int marker = current + 4; //move it past the block size data

			//move current to end of block
			int count = ReadInt(current, buffer);
			current += count + 3;

			//just getting one texture name
			while(buffer[marker] != 0)
			{
				textureName += buffer[marker];
				marker++;
			}
		}
			break;

		case 4: //character data
		{
			current++; //advance to size data
			//use marker to advance through this block
			int marker = current + 4; //move it past the block size data

			//move current to end of block
			int count = ReadInt(current, buffer);
			current += count + 3;

			int numChars = count / 20;
			for(int i = 0; i < numChars; ++i)
			{
				int charNum = ReadIntAndAdvance(marker, buffer);

				AngelcodeCharDescriptor AD;
				AD.x = ReadShortAndAdvance(marker, buffer);
				AD.y = ReadShortAndAdvance(marker, buffer);
				AD.width = ReadShortAndAdvance(marker, buffer);
				AD.height = ReadShortAndAdvance(marker, buffer);
				AD.xOffset = ReadShortAndAdvance(marker, buffer);
				AD.yOffset = -(float)ReadShortAndAdvance(marker, buffer);//negate y offsets for Blit3D coordinate system!
				AD.xAdvance = ReadShortAndAdvance(marker, buffer); 
				AD.lookupVerts = vertsCounter;
				if(Chars.count(charNum) == 0)
				{
					vertsCounter++; //only increment if unique key...turns out for some files we need this sanity check
					//add to map
					Chars[charNum] = AD;
				}
				
				marker += 2; //skip page & chnl data
			}
		}
			break;

		case 5: //kerning pairs data
		{
			current++; //advance to size data
			//use marker to advance through this block
			int marker = current + 4; //move it past the block size data

			//move current to end of block
			int count = ReadInt(current, buffer);
			current += count + 3;

			int numKerns = count / 10;
			
			std::unordered_map<int32_t, AngelcodeCharDescriptor>::iterator itr;

			for(int i = 0; i < numKerns; ++i)
			{
				uint32_t charNum1 = ReadUIntAndAdvance(marker, buffer);

				uint32_t charNum2 = ReadUIntAndAdvance(marker, buffer);

				int16_t amount = ReadShortAndAdvance(marker, buffer);

				//lookup second character, add first cahracter to it's kerning table
				itr = Chars.find(charNum2);
				if(itr != Chars.end())
				{
					itr->second.kerningTable[charNum1] = (float)amount;
				}

			}
		}
			break;

		default:
		{
			//skip unknown blocks for now
			current++; //advance to size data
			//move current to end of block
			int count = ReadInt(current, buffer);
			current += count + 3;
		}
			break;
		}//end switch
	}

	delete[] buffer;

	//Make a path string, so we can load textures from w/e the font file was
	std::string fontPath = DirectoryOfFilePath(fontfile);
	textureName = fontPath + textureName;
	texId = texManager->LoadTexture(textureName);

	verts = new B3D::TVertex[4 * Chars.size()]; //make an array of Textured Vertices

	// generate a new VAO and get the associated ID
	glGenVertexArrays(1, &vaoId); // Create our Vertex Array Object  
	glBindVertexArray(vaoId); // Bind our Vertex Array Object so we can use it  

	// generate a new VBO and get the associated ID
	glGenBuffers(1, &vboId);

	// bind VBO in order to use
	glBindBuffer(GL_ARRAY_BUFFER, vboId);

	//set the vertex array points...we need 4 vertices, one for each corner of our sprite,
	//per letter
	
	for(auto C : Chars)
	{
		//C.second.lookupVerts = loop; can't update this way
		int loop = C.second.lookupVerts;
		assert(loop < Chars.size());
		float cx = C.second.x;				// X Position Of Current Character
		float cy = C.second.y;				// Y Position Of Current Character
		
		float charwidth = C.second.width;
		float charheight = C.second.height;

		float xoffset = C.second.xOffset;
		float yoffset = C.second.yOffset - charheight; //invert char height for Blit3D coordinate system

		verts[loop * 4].x = 0 + xoffset; verts[loop * 4].y = 0 + yoffset;		// Vertex Coord (Bottom Left)
		verts[loop * 4].u = cx / scaleW;	verts[loop * 4].v = 1 - (cy + charheight) / scaleH;	// Texture Coord (Bottom Left)

		verts[loop * 4 + 1].x = xoffset + charwidth;	verts[loop * 4 + 1].y = 0 + yoffset;	// Vertex Coord (Bottom Right)	
		verts[loop * 4 + 1].u = (cx + charwidth) / scaleW;	verts[loop * 4 + 1].v = 1 - (cy + charheight) / scaleH;	// Texture Coord (Bottom Right)

		verts[loop * 4 + 2].x = xoffset + charwidth;	verts[loop * 4 + 2].y = yoffset + charheight;	// Vertex Coord (Top Right)
		verts[loop * 4 + 2].u = (cx + charwidth) / scaleW;	verts[loop * 4 + 2].v = 1 - cy / scaleH;	// Texture Coord (Top Right)

		verts[loop * 4 + 3].x = 0 + xoffset;	verts[loop * 4 + 3].y = yoffset + charheight;		// Vertex Coord (Top Left)							
		verts[loop * 4 + 3].u = cx / scaleW;	verts[loop * 4 + 3].v = 1 - cy / scaleH;	// Texture Coord (Top Left)

		verts[loop * 4].z = verts[loop * 4 + 1].z = verts[loop * 4 + 2].z = verts[loop * 4 + 3].z = 0.f;
	}

	// upload data to VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(B3D::TVertex) * 4 * Chars.size(), verts, GL_STATIC_DRAW);

	// Set up our vertex attributes pointers
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(B3D::TVertex), BUFFER_OFFSET(0)); //3 values (x,y,z) per point, start at 0 offset 
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(B3D::TVertex), BUFFER_OFFSET(sizeof(GLfloat) * 3)); //Start after x,y,z, data 



	// activate attribute array
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2); // don'yt use channel 2
	glDisableVertexAttribArray(3); //don't use Color channel, we are textured

	glBindVertexArray(0); // Disable our Vertex Array Object? 
	glBindBuffer(GL_ARRAY_BUFFER, 0);// Disable our Vertex Buffer Object

	//free the memory once it's been uploaded
	delete[] verts;
}

AngelcodeFont::~AngelcodeFont()
{
	// free texture
	texManager->FreeTexture(textureName);

	// delete VBO when object destroyed
	glDeleteBuffers(1, &vboId);
	glDeleteVertexArrays(1, &vaoId);
}

//draws the string
void AngelcodeFont::BlitText(float x, float y, std::string output)
{
	dest_x = x;
	dest_y = y;

	glBindVertexArray(vaoId); // Bind our Vertex Array Object 

	//bind our texture
	texManager->BindTexture(texId);

	// set the translation matrix
	modelMatrix = glm::translate(glm::mat4(1.f), glm::vec3(dest_x, dest_y, 0.f));
	//apply rotation
	modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.f, 0.f, 1.f));

	//send our alpha to the shader
	prog->setUniform("in_Alpha", alpha);

	//send our modelMatrix to the shader
	prog->setUniform("modelMatrix", modelMatrix);
	prog->setUniform("in_Scale_X", 1.f); //default scaling
	prog->setUniform("in_Scale_Y", 1.f); //default scaling
	
	std::unordered_map<int32_t, AngelcodeCharDescriptor>::iterator itr;
	std::unordered_map<int32_t, float>::iterator itrK;

	int prevLetter = -1; //shouldn't find a kerning pair for this letter on first pass

	for(unsigned int i = 0; i < output.size(); ++i)
	{
		//lookup this letter in the Chars map and fetch its verts offset
		itr = Chars.find(output[i]);
		if(itr != Chars.end())
		{ 
			//kerning: lookup previous letter in cyrrent character's kerning map
			itrK = itr->second.kerningTable.find(prevLetter);
			if(itrK != itr->second.kerningTable.end())
			{
				modelMatrix = glm::translate(modelMatrix, glm::vec3(itrK->second, 0.f, 0.f));
			}
			
			// draw a quad: 1 quad x 4points per quad = 4 verts, the third argument
			glDrawArrays(GL_QUADS, itr->second.lookupVerts * 4, 4);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(itr->second.xAdvance, 0.f, 0.f));
			prog->setUniform("modelMatrix", modelMatrix);
			prevLetter = output[i]; //store this letter for kerning the next one
		}
	}

	// bind with 0, so, switch back to normal pointer operation
	glBindVertexArray(0);

	return;
}

//returns the width of the text string, in pixels
float AngelcodeFont::WidthText(std::string output)
{
	float width_text = 0;
	std::unordered_map<int32_t, AngelcodeCharDescriptor>::iterator itr;
	std::unordered_map<int32_t, float>::iterator itrK;

	int prevLetter = -1; //shouldn't find a kerning pair for this letter on first pass

	for(unsigned int i = 0; i < output.size(); ++i)
	{

		itr = Chars.find(output[i]);
		if(itr != Chars.end())
		{
			//kerning: lookup previous letter in current character's kerning map
			itrK = itr->second.kerningTable.find(prevLetter);
			if(itrK != itr->second.kerningTable.end())
			{
				width_text += (itr->second.xAdvance + itrK->second);
			}
			else width_text += itr->second.xAdvance;

			prevLetter = output[i]; //store this letter for kerning the next one
		}
	}

	return width_text;
}

int16_t AngelcodeFont::ReadShortAndAdvance(int &offset, char buffer[])
{
	int16_t *temp = (int16_t *)&buffer[offset];
	int16_t val = *temp;
	
	switch(endian)
	{
	case 0://little-endian
		//
		break;

	case 1: //big-endian	
		val = swap_int16(val);
		break;
	case 2:
		break;
	}	
	
	offset += 2;
	return val;
}

int32_t AngelcodeFont::ReadIntAndAdvance(int &offset, char buffer[])
{	
	int32_t *charNum = (int32_t *)&buffer[offset];
	int32_t val = *charNum; 	
	
	switch(endian)
	{
	case 0://little-endian
		
		break;

	case 1: //big-endian	
		val = swap_int32(val);		
		break;
	case 2: //middle-endian??
		break;
	}

	offset += 4;
	return val;
}

uint32_t AngelcodeFont::ReadUIntAndAdvance(int &offset, char buffer[])
{
	uint32_t *charNum = (uint32_t *)&buffer[offset];
	uint32_t val = *charNum;

	switch(endian)
	{
	case 0://little-endian

		break;

	case 1: //big-endian	
		val = swap_uint32(val);
		break;
	case 2: //middle-endian??
		break;
	}

	offset += 4;
	return val;
}

int16_t AngelcodeFont::ReadShort(int offset, char buffer[])
{
	int16_t *temp = (int16_t *)&buffer[offset];
	int16_t val = *temp;

	switch(endian)
	{
	case 0://little-endian
		//
		break;

	case 1: //big-endian	
		val = swap_int16(val);
		break;
	case 2:
		break;
	}	
	return val;
}

int32_t AngelcodeFont::ReadInt(int offset, char buffer[])
{
	int32_t *charNum = (int32_t *)&buffer[offset];
	int32_t val = *charNum;

	switch(endian)
	{
	case 0://little-endian

		break;

	case 1: //big-endian	
		val = swap_int32(val);
		break;
	case 2: //middle-endian??
		break;
	}
	
	return val;
}