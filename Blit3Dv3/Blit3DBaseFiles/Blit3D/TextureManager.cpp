#include "TextureManager.h"
#include <iostream>
#include "Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//use the main Blit3D logger
extern logger oLog;

TextureManager::TextureManager(void)
{
	
	for (int i = 0; i < TEXTURE_MANAGER_MAX_TEXTURES; ++i) currentId[i] = -1;

	texturePath = "";

	//try for nicest mipmap generation
	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST );

	//tell stb to flip the image (OpenGL stores textures "upside-down")
	stbi_set_flip_vertically_on_load(true);

	oLog(Level::Info) << "Creating TextureManager with " << TEXTURE_MANAGER_MAX_TEXTURES << " maximum bound textures";
}

TextureManager::~TextureManager(void)
{
	//free all our textures
	for(itor = textures.begin(); itor != textures.end(); itor++)
	{		
		glDeleteTextures( 1, &(*itor->second).texId); //free the texture memory used by OpenGL
		delete (itor)->second; //free the instance of a tex struct
	}

	textures.clear(); //free the map

	oLog(Level::Info) << "TextureManager destroyed";
}

void TextureManager::SetTexturePath(std::string path)
{
	texturePath = path;
	oLog(Level::Info) << "Texture Path set to: " << path;
}

GLuint TextureManager::LoadTexture(std::string filename, bool useMipMaps, GLuint texture_unit, GLuint wrapflag, bool pixelate)
{
	itor = textures.find(filename); //lookup this texture in our std::map

	if(itor == textures.end())
	{
		//we didn't find that texture name, so it is a new texture
		tex *newtex = new tex;

		newtex->refcount = 1;
		newtex->unload = true; //currently setting all textures to unload when refcount = 0;

		//add the path to the file
		std::string fullpath;
		fullpath = texturePath;
		fullpath.append(filename);

		//pointer to the image data
		BYTE* bits(0);
		//image width and height, and #of components (1= gray scale, 4 = rgba)
		int width(0), height(0), components(0);
		//OpenGL's image ID to map to
		GLuint gl_texID;

		//retrieve the image data, currently force to RGBA (4 components)
		bits = stbi_load(filename.c_str(), &width, &height, &components, 4);
		
		//if somehow one of these failed (they shouldn't), return failure
		if((bits == 0) || (width == 0) || (height == 0))
		{
			if(bits == 0) oLog(Level::Severe) << "bits = 0";
			if(width == 0) oLog(Level::Severe) << "width = 0";
			if(height == 0) oLog(Level::Severe) << "height = 0";
			goto ERROR_HANDLER;
		}
		
		//generate an OpenGL texture ID for this texture
		glGenTextures(1, &gl_texID);
		//store the texture ID mapping
		newtex->texId = gl_texID;
		
		glActiveTexture(texture_unit); //needed for programmable shaders?
		//bind to the new texture ID
		glBindTexture(GL_TEXTURE_2D, gl_texID);

		//set up some vars for OpenGL texturizing
		GLenum image_format = GL_RGBA;
		GLint internal_format = GL_RGBA;
		GLint level = 0;
		//store the texture data for OpenGL use
		glTexImage2D(GL_TEXTURE_2D, level, internal_format, width, height,
			0, image_format, GL_UNSIGNED_BYTE, bits);

		//swizzle colors - not needed for stb_image
		//GLint swizzleMask[] = { GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA };
		//glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

		if (useMipMaps)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		//Free stb's copy of the data
		stbi_image_free(bits);

		newtex->width = width;
		newtex->height = height;
		
		//add the new texture to the map
		textures[filename] = newtex;		

		currentId[texture_unit - GL_TEXTURE0] = newtex->texId;

		//setup texture filtering for when we are close/far away
		if (useMipMaps)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //for when we are close
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);//when we are far away
		}
		else if(pixelate)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //for when we are close
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//when we are far away
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //for when we are close
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//when we are far away
		}


		//the following turns on a special, high-quality filtering mode called "ANISOTROPY"
		if(GL_EXT_texture_filter_anisotropic)
		{
			GLfloat largest_supported_anisotropy;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largest_supported_anisotropy);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largest_supported_anisotropy);
		}

		// the texture stops at the edges with GL_CLAMP_TO_EDGE
		//...experiment with GL_CLAMP and GL_REPEAT as well
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapflag );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapflag );
		
		//return the loaded texture object
		return newtex->texId;
	}
	
	//if we get here in the code, we already had that texture loaded by some other object
	(*itor->second).refcount++; //update the reference counter
	//bind it to the texture unit
	BindTexture((*itor->second).texId, texture_unit);
	return (*itor->second).texId;//and return the OpenGL texture object ID associated with that texture

ERROR_HANDLER:
	oLog(Level::Severe) << "ERROR loading file: " << filename;
	assert(false && "ERROR loading file");
	return 0;
}

void TextureManager::FreeTexture(std::string filename)
{
	itor = textures.find(filename); //lookup this texture in our std::map

	if(itor != textures.end())
	{
		(*itor->second).refcount--; //update the refcount
		if((*itor->second).refcount <= 0 && (*itor->second).unload)
		{
			//we have freed the last refernce, so we can delete this texture from memory
			glDeleteTextures(1, &(*itor->second).texId);

			//if this was the currently bound texture, set currentId to a bad ID value
			//that won't be matched by the next call to BindTexture()
			for (int i = 0; i < TEXTURE_MANAGER_MAX_TEXTURES; ++i)
				if (currentId[i] == (*itor->second).texId) currentId[i] = -1;

			delete (itor)->second; //free the instance of a tex struct
			//clear the texture from the std::unordered_map
			textures.erase(itor);
		}
	}
	else oLog(Level::Warning) << "Tried to free texture " << filename << " but it isn't loaded currently";
}

void TextureManager::BindTexture(GLuint bindId, GLuint texture_unit)
{
	//We only call glBindTexture if the texture is NOT the last one bound.
	//On some driver implementations, calling glBindTexture() with the 
	//currently bound texture object will be a performance hit, like
	//ACTUALLY changing textures is a performance hit. 

	if (currentId[texture_unit - GL_TEXTURE0] != bindId)
	{
		glActiveTexture(texture_unit); //needed for programmable shaders
		glBindTexture(GL_TEXTURE_2D, bindId);
		
		currentId[texture_unit - GL_TEXTURE0] = bindId;
	}
}

void TextureManager::BindTexture(std::string filename, GLuint texture_unit)
{
	itor = textures.find(filename); //lookup this texture in our std::unordered_map

	if (itor != textures.end())
	{
		BindTexture((*itor->second).texId, texture_unit);
		return;
	}

	//didn't find it in the list of loaded textures, so load it.
	//LoadTexture() binds it when it loads.
	LoadTexture(filename, true, GL_CLAMP_TO_EDGE, texture_unit);
}

void TextureManager::InitShaderVar(GLSLProgram *the_shader, const char *samplerName, int shaderVar)
{
	the_shader->setUniform(samplerName, shaderVar);
}

void TextureManager::AddLoadedTexture(std::string name, GLuint bindId)
{
	itor = textures.find(name); //lookup this texture in our std::map

	if (itor == textures.end())
	{
		tex *newtex = new tex;

		newtex->refcount = 1;
		newtex->unload = true; //currently setting all textures to unload when refcount = 0;

		newtex->texId = bindId;
		textures[name] = newtex;
	}
	else
	{
		//update the ref count
		(*itor->second).refcount++; //update the reference counter
	}
}

bool TextureManager::FetchDimensions(std::string name, GLfloat &width, GLfloat &height)
{
	itor = textures.find(name); //lookup this texture in our std::map

	if(itor != textures.end())
	{
		width = static_cast<float>((*itor->second).width);
		height = static_cast<float>((*itor->second).height);
		return true;
	}

	//didn't find it in the list of loaded textures
	oLog(Level::Warning) << "File: " << name << "is not loaded, so cannot fetch dimensions";
	return false;
}