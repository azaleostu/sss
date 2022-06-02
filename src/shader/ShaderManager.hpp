#ifndef SHADER_MANAGER
#define SHADER_MANAGER

#include "Shader.hpp"
#include "imgui.h"
#include <glad/glad.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>

class ShaderManager
{
  public:
	std::vector<shader> shaders;
	std::string			folder;
	GLuint				program = GL_INVALID_INDEX;

	ShaderManager() = delete;
	ShaderManager( const std::string & folder ) : folder( folder ) {}
	~ShaderManager();
	void	 addShader( const std::string & name, const GLenum & type, const std::string & file );
	shader * getShader( const std::string & name );
	void	 use( GLuint & prgrm );
	void	 link();
	void	 init() { program = glCreateProgram(); }
	// set functions
	void setBool( const std::string & name, bool value ) const;
	void setInt( const std::string & name, int value ) const;
	void setFloat( const std::string & name, float value ) const;
	void setVec2( const std::string & name, const glm::vec2 & value ) const;
	void setVec2( const std::string & name, float x, float y ) const;
	void setVec3( const std::string & name, const glm::vec3 & value ) const;
	void setVec3( const std::string & name, float x, float y, float z ) const;
	void setVec4( const std::string & name, const glm::vec4 & value ) const;
	void setVec4( const std::string & name, float x, float y, float z, float w );
	void setMat2( const std::string & name, const glm::mat2 & mat ) const;
	void setMat3( const std::string & name, const glm::mat3 & mat ) const;
	void setMat4( const std::string & name, const glm::mat4 & mat ) const;
};

#endif
