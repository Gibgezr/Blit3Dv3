/* GLSLProgram is based on code from  the excellent text "OpenGL 4.0 Shading Language Cookbook"
	by David Wolff.
	Modified by Darren Reid to suit Blit3D needs.

	Version 1.1 added support for vec2 uniforms
	Version 1.0	added a map for uniform/attributes, to cache lookup of locations in shader
*/

#pragma once
#include <GL/glew.h>
#include <GL/gl.h>

#include <string>
using std::string;

#include <glm/glm.hpp>
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::mat3;

#include <map>

namespace GLSLShader {
    enum GLSLShaderType {
        VERTEX, FRAGMENT, GEOMETRY,
        TESS_CONTROL, TESS_EVALUATION
    };
};

class GLSLProgram
{
private:
    int  handle;
    bool linked;
    string logString;

    int  getUniformLocation(const char * name );
    bool fileExists( const string & fileName );

	//Store uniforms and attributes in a map for easy lookup
	std::map<std::string, int> UniformMap;
	std::map<std::string, int>::iterator UMapIter;

public:
    GLSLProgram();
	~GLSLProgram();

    bool   compileShaderFromFile( const char * fileName, GLSLShader::GLSLShaderType type );
    bool   compileShaderFromString( const string & source, GLSLShader::GLSLShaderType type );
    bool   link();
    void   use();

    string log();

    int    getHandle();
    bool   isLinked();

    void   bindAttribLocation( GLuint location, const char * name);
    void   bindFragDataLocation( GLuint location, const char * name );
	
	void   setUniform(const char *name, float x, float y);
    void   setUniform( const char *name, float x, float y, float z);
	void   setUniform( const char *name, const vec2 & v);
    void   setUniform( const char *name, const vec3 & v);
    void   setUniform( const char *name, const vec4 & v);
    void   setUniform( const char *name, const mat4 & m);
    void   setUniform( const char *name, const mat3 & m);
    void   setUniform( const char *name, float val );
    void   setUniform( const char *name, int val );
    void   setUniform( const char *name, bool val );

    void   printActiveUniforms();
    void   printActiveAttribs();

	int GetUniform(const char* name);
	int GetAttribute(const char* name);
};

