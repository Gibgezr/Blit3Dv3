#include "glslprogram.h"

#include "glutils.h"

#include <fstream>
using std::ifstream;
using std::ios;

#include <sstream>
using std::ostringstream;

#include <sys/stat.h>

GLSLProgram::GLSLProgram() : handle(0), linked(false) { }

GLSLProgram::~GLSLProgram()
{
	if (handle)
	{
		glDeleteProgram(handle);
	}
}

bool GLSLProgram::compileShaderFromFile( const char * fileName,
                                         GLSLShader::GLSLShaderType type )
{
    if( ! fileExists(fileName) )
    {
        logString = "File not found.";
		assert(false);
        return false;
    }

    if( handle <= 0 ) 
	{
        handle = glCreateProgram();
        if( handle == 0) 
		{
            logString = "Unable to create shader program.";
			assert(false);
            return false;
        }
    }

    ifstream inFile( fileName, ios::in );
    if( !inFile ) 
	{
		assert(false);
        return false;
    }

    ostringstream code;
    while( inFile.good() ) 
	{
        int c = inFile.get();
        if( ! inFile.eof() ) code << (char) c;
    }
    inFile.close();

    return compileShaderFromString(code.str(), type);
}

bool GLSLProgram::compileShaderFromString( const string & source, GLSLShader::GLSLShaderType type )
{
    if( handle <= 0 ) 
	{
        handle = glCreateProgram();
        if( handle == 0) 
		{
            logString = "Unable to create shader program.";
			assert(false);
            return false;
        }
    }

    GLuint shaderHandle = 0;

    switch( type ) 
	{
    case GLSLShader::VERTEX:
        shaderHandle = glCreateShader(GL_VERTEX_SHADER);
        break;
    case GLSLShader::FRAGMENT:
        shaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
        break;
    case GLSLShader::GEOMETRY:
        shaderHandle = glCreateShader(GL_GEOMETRY_SHADER);
        break;
    case GLSLShader::TESS_CONTROL:
        shaderHandle = glCreateShader(GL_TESS_CONTROL_SHADER);
        break;
    case GLSLShader::TESS_EVALUATION:
        shaderHandle = glCreateShader(GL_TESS_EVALUATION_SHADER);
        break;
    default:
        return false;
    }

    const char * c_code = source.c_str();
    glShaderSource( shaderHandle, 1, &c_code, NULL );

    // Compile the shader
    glCompileShader(shaderHandle );

    // Check for errors
    int result;
    glGetShaderiv( shaderHandle, GL_COMPILE_STATUS, &result );
    if( GL_FALSE == result ) 
	{
        // Compile failed, store log and return false
        int length = 0;
        logString = "";
        glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &length );
        if( length > 0 ) 
		{
            char * c_log = new char[length];
            int written = 0;
            glGetShaderInfoLog(shaderHandle, length, &written, c_log);
            logString = c_log;
            delete [] c_log;
        }
		assert(false);
        return false;
    } 
	else 
	{
        // Compile succeeded, attach shader and return true
        glAttachShader(handle, shaderHandle);
        return true;
    }
}

bool GLSLProgram::link()
{
    if( linked ) return true;
    if( handle <= 0 ) return false;

    glLinkProgram(handle);

    int status = 0;
    glGetProgramiv( handle, GL_LINK_STATUS, &status);
    if( GL_FALSE == status ) 
	{
        // Store log and return false
        int length = 0;
        logString = "";

        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length );

        if( length > 0 ) 
		{
            char * c_log = new char[length];
            int written = 0;
            glGetProgramInfoLog(handle, length, &written, c_log);
            logString = c_log;
            delete [] c_log;
        }
		assert(false);
        return false;
    } 
	else 
	{
        linked = true;
        return linked;
    }
}

void GLSLProgram::use()
{
	if(handle <= 0 || (!linked))
	{
		assert(handle > 0 && linked);
		return;
	}

    glUseProgram( handle );
}

string GLSLProgram::log()
{
    return logString;
}

int GLSLProgram::getHandle()
{
    return handle;
}

bool GLSLProgram::isLinked()
{
    return linked;
}

void GLSLProgram::bindAttribLocation( GLuint location, const char * name)
{
    glBindAttribLocation(handle, location, name);
}

void GLSLProgram::bindFragDataLocation( GLuint location, const char * name )
{
    glBindFragDataLocation(handle, location, name);
}

void GLSLProgram::setUniform(const char *name, float x, float y)
{
	int loc = getUniformLocation(name);
	assert(loc >= 0 && "setUniform failed");
	if (loc >= 0) 
	{
		glUniform2f(loc, x, y);
	}
}

void GLSLProgram::setUniform( const char *name, float x, float y, float z)
{
    int loc = getUniformLocation(name);
	assert(loc >= 0 && "setUniform failed");
    if( loc >= 0 ) 
	{
        glUniform3f(loc,x,y,z);
    }
}

void GLSLProgram::setUniform(const char *name, const vec2 & v)
{
	this->setUniform(name, v.x, v.y);
}

void GLSLProgram::setUniform( const char *name, const vec3 & v)
{
    this->setUniform(name,v.x,v.y,v.z);
}

void GLSLProgram::setUniform( const char *name, const vec4 & v)
{
    int loc = getUniformLocation(name);
	assert(loc >= 0 && "setUniform failed");
    if( loc >= 0 ) 
	{
        glUniform4f(loc,v.x,v.y,v.z,v.w);
    }
}

void GLSLProgram::setUniform( const char *name, const mat4 & m)
{
    int loc = getUniformLocation(name);
	assert(loc >= 0 && "setUniform failed");
    if( loc >= 0 )
    {
        glUniformMatrix4fv(loc, 1, GL_FALSE, &m[0][0]);
    }
}

void GLSLProgram::setUniform( const char *name, const mat3 & m)
{
    int loc = getUniformLocation(name);
	assert(loc >= 0 && "setUniform failed");
    if( loc >= 0 )
    {
        glUniformMatrix3fv(loc, 1, GL_FALSE, &m[0][0]);
    }
}

void GLSLProgram::setUniform( const char *name, float val )
{
    int loc = getUniformLocation(name);
	assert(loc >= 0 && "setUniform failed");
    if( loc >= 0 )
    {
        glUniform1f(loc, val);
    }
}

void GLSLProgram::setUniform( const char *name, int val )
{
    int loc = getUniformLocation(name);
	assert(loc >= 0 && "setUniform failed");
    if( loc >= 0 )
    {
        glUniform1i(loc, val);
    }
}

void GLSLProgram::setUniform( const char *name, bool val )
{
    int loc = getUniformLocation(name);
	assert(loc >= 0 && "setUniform failed");
    if( loc >= 0 )
    {
        glUniform1i(loc, val);
    }
}

void GLSLProgram::printActiveUniforms() {

    GLint nUniforms, size, location, maxLen;
    GLchar * name;
    GLsizei written;
    GLenum type;

    glGetProgramiv( handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);
    glGetProgramiv( handle, GL_ACTIVE_UNIFORMS, &nUniforms);

    name = (GLchar *) malloc( maxLen );

    printf(" Location | Name\n");
    printf("------------------------------------------------\n");
    for( int i = 0; i < nUniforms; ++i ) 
	{
        glGetActiveUniform( handle, i, maxLen, &written, &size, &type, name );
        location = glGetUniformLocation(handle, name);
        printf(" %-8d | %s\n",location, name);
    }

    free(name);
}

void GLSLProgram::printActiveAttribs() {

    GLint written, size, location, maxLength, nAttribs;
    GLenum type;
    GLchar * name;

    glGetProgramiv(handle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);
    glGetProgramiv(handle, GL_ACTIVE_ATTRIBUTES, &nAttribs);

    name = (GLchar *) malloc( maxLength );

    printf(" Index | Name\n");
    printf("------------------------------------------------\n");
    for( int i = 0; i < nAttribs; i++ ) 
	{
        glGetActiveAttrib( handle, i, maxLength, &written, &size, &type, name );
        location = glGetAttribLocation(handle, name);
        printf(" %-5d | %s\n",location, name);
    }

    free(name);
}

int GLSLProgram::getUniformLocation(const char * name )
{
	//return glGetUniformLocation(handle, name);
	return GetUniform(name);
}

bool GLSLProgram::fileExists( const string & fileName )
{
    struct stat info;
    int ret = -1;

    ret = stat(fileName.c_str(), &info);
    return 0 == ret;
}

int GLSLProgram::GetUniform(const char* name)
{
	int result = -1;

	if (linked)
	{
		UMapIter = UniformMap.find(name);
		if (UMapIter == UniformMap.end())
		{
			result = glGetUniformLocation(handle, name);
			
			if (result != -1) UniformMap[name] = result;
		}
		else
		{		
			result = UMapIter->second;
		}
	}

	return result;
}

int GLSLProgram::GetAttribute(const char* name)
{
	int result = -1;

	if (linked)
	{
		UMapIter = UniformMap.find(name);
		if (UMapIter == UniformMap.end())
		{
			result = glGetAttribLocation(handle, name);

			if (result != -1) UniformMap[name] = result;
		}
		else
		{
			result = UMapIter->second;
		}
	}

	return result;
}