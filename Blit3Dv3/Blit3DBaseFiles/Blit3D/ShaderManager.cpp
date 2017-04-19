#include "ShaderManager.h"
#include "Logger.h"
#include <cassert>

//use the main Blit3D logger
extern logger oLog;

ShaderManager::~ShaderManager()
{
	for (auto item : ShaderMap)
	{
		delete item.second;
	}
}

GLSLProgram* ShaderManager::Load(const char* vertName, const char*fragName)
{
	GLSLProgram* prog = new GLSLProgram();
	
	if (!prog->compileShaderFromFile(vertName, GLSLShader::VERTEX))
	{
		printf("Vertex shader failed to compile!\n%s", prog->log().c_str());
		oLog(Level::Severe) << "Vertex shader <" << vertName << "> failed to compile." << prog->log();
		assert(false && "Vertex shader failed to compile");
		return NULL;
	}

	if (!prog->compileShaderFromFile(fragName, GLSLShader::FRAGMENT))
	{
		printf("Fragment shader failed to compile!\n%s", prog->log().c_str());
		oLog(Level::Severe) << "Fragment shader <" << fragName << "> failed to compile." << prog->log();
		assert(false && "Fragment shader failed to compile");
		return NULL;
	}

	if (!prog->link())
	{
		printf("Shader program failed to link!\n%s", prog->log().c_str());
		oLog(Level::Severe) << "Shader program failed to link." << prog->log();
		assert(false && "Shader program failed to link.");
		return NULL;
	}

	assert(prog != NULL);
	return prog;
}

GLSLProgram* ShaderManager::LoadFromStrings(const char* vertName, const char*fragName, std::string vertString, std::string fragString)
{
	GLSLProgram* prog = new GLSLProgram();

	if(!prog->compileShaderFromString(vertString, GLSLShader::VERTEX))
	{
		printf("Vertex shader failed to compile from string!\n%s", prog->log().c_str());
		oLog(Level::Severe) << "Vertex shader <" << vertName << "> failed to compile from string." << prog->log();
		assert(false && "Vertex shader failed to compile from string");
		return NULL;
	}

	if(!prog->compileShaderFromString(fragString, GLSLShader::FRAGMENT))
	{
		printf("Fragment shader failed to compile from string!\n%s", prog->log().c_str());
		oLog(Level::Severe) << "Fragment shader <" << fragName << "> failed to compile from string." << prog->log();
		assert(false && "Fragment shader failed to compile from string");
		return NULL;
	}

	if(!prog->link())
	{
		printf("Shader program failed to link!\n%s", prog->log().c_str());
		oLog(Level::Severe) << "Shader program failed to link." << prog->log();
		assert(false && "Shader program failed to link.");
		return NULL;
	}

	assert(prog != NULL);
	return prog;
}

GLSLProgram* ShaderManager::GetShader(const char* vertName, const char* fragName)
{
	std::string key = vertName;
	key += fragName;


	shaderIter = ShaderMap.find(key);
	if (shaderIter != ShaderMap.end())
	{
		return (shaderIter->second);
	}
	else
	{
		GLSLProgram* prog = Load(vertName, fragName);
		if (prog != NULL)
		{
			// successful loaded and linked shader program, added to map and return the result
			ShaderMap[key] = prog;
			assert(prog != NULL);
			return prog;
		}
		assert(prog != NULL);
	}

	// ERROR
	return NULL;
}

GLSLProgram* ShaderManager::GetShader(const char* vertName, const char* fragName, std::string vertString, std::string fragString)
{
	std::string key = vertName;
	key += fragName;


	shaderIter = ShaderMap.find(key);
	if(shaderIter != ShaderMap.end())
	{
		return (shaderIter->second);
	}
	else
	{
		GLSLProgram* prog = LoadFromStrings(vertName, fragName, vertString, fragString);
		if(prog != NULL)
		{
			// successful loaded and linked shader program, added to map and return the result
			ShaderMap[key] = prog;
			assert(prog != NULL);
			return prog;
		}
		assert(prog != NULL);
	}

	// ERROR
	return NULL;
}

GLSLProgram* ShaderManager::UseShader(const char* vertName, const char* fragName)
{
	GLSLProgram* prog = GetShader(vertName, fragName);
	assert(prog != NULL);
	prog->use();
	return prog;
}

GLSLProgram* ShaderManager::UseShader(const char* vertName, const char* fragName, std::string vertString, std::string fragString)
{
	GLSLProgram* prog = GetShader(vertName, fragName,vertString, fragString);
	assert(prog != NULL);
	prog->use();
	return prog;
}