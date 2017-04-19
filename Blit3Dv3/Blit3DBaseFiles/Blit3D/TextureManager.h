/*
Darren's Texture Manager

Example texture manager, demonstrates reference counting and
texture binding state management.

Now uses the excellent stb_image library as it's image loader.

Version 3.1, get stb to flip imges as it loads them so that they are right-side up in OpenGL
Version 3.0, uses stb_image instead of FreeImage (no more fake memory leaks etc)
Version 2.3, uses GLEW on all platforms for now
Version 2.2, changed from std::map to std::unordered_map for better speed
Version 2.1, added pixelate argument to LoadTexture() for pixel graphics
Version 2.0, added support for multi-texturing
Version 1.8, added BindTexture(std::string filename) to bind by filename
Version 1.7, uses Free Image instead of SOIL as it's image loading library
Version 1.6, wrap type defaults to GL_CLAMP_TO_EDGE

Version 1.5, added all calls the glTexParameter funcs inside the TextureManager,
and changed LoadTexture() so that it takes an argument for texture wrap type

Version 1.4, added AddLoadedTexture() to let FBO add textures to the TM

*/

#pragma once

#ifdef _WIN32 
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

//#define GLEW_STATIC
#include <GL/glew.h> // include GLEW and new version of GL on Windows
#include <GLFW/glfw3.h> // GLFW helper library

#include <unordered_map>
#include <algorithm>
#include "glslprogram.h"

struct tex
{
public:
	GLuint texId; //opengl texture object id
	GLuint refcount; //reference counter...how many objects are using this texture
	bool unload; //do we unload this texture and free it's id when the refcount is 0?
	int width, height;
};

//the maximum texture units OpenGL supports
#define TEXTURE_MANAGER_MAX_TEXTURES 31

// This object will allocate, track references, and free all textures
class TextureManager
{
private:
	std::unordered_map<std::string, tex *> textures; //list of textures and associated id's, in a hashmap
	GLuint currentId[TEXTURE_MANAGER_MAX_TEXTURES]; //currently bound texture
	std::unordered_map<std::string, tex *>::iterator itor; //might as well save an iterator to use on our map
	
public:
	std::string texturePath; //relative path to the files

	int texureLocation; // Store the location of our texture sampler in the shader

	void InitShaderVar(GLSLProgram *the_shader, const char * samplerName, int shaderVar = 0); //initalizes the shader variable for the sampler

	GLuint LoadTexture(std::string filename, bool useMipMaps = false, GLuint texture_unit = GL_TEXTURE0, GLuint wrapflag = GL_CLAMP_TO_EDGE, bool pixelate = true);
	void FreeTexture(std::string filename); 
	void BindTexture(GLuint bindId, GLuint texture_unit = GL_TEXTURE0);
	void BindTexture(std::string filename, GLuint texture_unit = GL_TEXTURE0);
	void SetTexturePath(std::string path);
	void AddLoadedTexture(std::string name, GLuint bindId);//used by FBO add pre-created textures
	bool FetchDimensions(std::string name, GLfloat &width, GLfloat &height);
	TextureManager(void);
	~TextureManager(void);
};


