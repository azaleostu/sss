#include "Shader.hpp"

// default constructor needs file-content and shader-type
shader::shader( const std::string& name, const std::string & shaderContentStr, const GLenum & type ) :
	name(name), shaderContentStr( shaderContentStr.c_str() ), id( glCreateShader( type ) )
{
	glShaderSource( id, 1, &this->shaderContentStr, NULL );
}

// default destructor
shader::~shader() { glDeleteShader( id ); }

// delete shader
void shader::del() { glDeleteShader( id ); }

// compile shader and check compilation
bool shader::compile()
{
	glCompileShader( id );
	GLint compiledf;
	glGetShaderiv( id, GL_COMPILE_STATUS, &compiledf );
	if ( !compiledf )
	{
		GLchar log[ 1024 ];
		glGetShaderInfoLog( id, sizeof( log ), NULL, log );
		glDeleteShader( id );
		std ::cerr << " Error compiling vertex shader : " << log << std ::endl;
		return false;
	}
	return true;
}

// attach shader to program
void shader::attachToProgram( const GLuint & programId ) { glAttachShader( programId, id ); }

// detach shader from program
void shader::detachFromPorgram( const GLuint & programId ) { glDetachShader( programId, id ); }