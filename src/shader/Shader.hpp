#ifndef SHADER_H
#define SHADER_H

#include <imgui.h>
#include <glad/glad.h>
#include <string>
#include <iostream>

class shader
{
  private:
	const GLchar * shaderContentStr; // shader-file content
  public:
	const GLuint id; // shader id
	const std::string name;

	// empty constructor deprecated
	shader() = delete;

	// default constructor needs file-content and shader-type
	shader( const std::string & name, const std::string & shaderContentStr, const GLenum & type );

	// default destructor
	~shader();

	// delete shader
	void del();

	// compile shader and check compilation
	bool compile();

	// attach shader to program
	void attachToProgram( const GLuint & programId );

	// detach shader from program
	void detachFromPorgram( const GLuint & programId );
};

#endif // !SHADER_H
