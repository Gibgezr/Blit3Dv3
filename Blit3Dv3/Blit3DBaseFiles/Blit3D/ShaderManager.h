/*
	Darren's Shader Manager
	TODO:	make ShaderManager store individual compiled shaders and look them up when linking,
			so that progs can re-use vert or frag shaders without recompiling?

	Version 1.1
*/

#pragma once

#include "glslprogram.h"

class ShaderManager
{
private:
	std::map<std::string, GLSLProgram*> ShaderMap;

	GLSLProgram* Load(const char* vertName, const char*fragName);
	GLSLProgram*LoadFromStrings(const char* vertName, const char*fragName, std::string vertString, std::string fragString);

	std::map<std::string, GLSLProgram*>::iterator shaderIter;

public:
	//Try to retrive a shader: if none exists for this combination or vert and frag shaders, load and 
	//compile and link it, then store on map
	GLSLProgram* GetShader(const char* vertName, const char* fragName);
	GLSLProgram* GetShader(const char* vertName, const char* fragName, std::string vertString, std::string fragString);
	//binds a shader for use
	GLSLProgram* UseShader(const char* vertName, const char* fragName);
	GLSLProgram* UseShader(const char* vertName, const char* fragName, std::string vertString, std::string fragString);

	~ShaderManager();
};
