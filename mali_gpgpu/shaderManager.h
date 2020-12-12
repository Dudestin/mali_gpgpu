#pragma once

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#include <iostream>

#include "macro.h"

class shaderManager
{
	GLuint vertexShader;
	GLuint fragmentShader;
public:
	GLuint glslProgram;

	shaderManager(const GLchar* vtx_s, const GLchar* flg_s)
	{
		this->glslProgram = glCreateProgram();
		this->vertexShader = glCreateShader(GL_VERTEX_SHADER);
		this->fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

		// set source for shader
		glShaderSource(this->vertexShader, 1, &vtx_s, NULL);
		glShaderSource(this->fragmentShader, 1, &flg_s, NULL);
		// compile shader
		glCompileShader(this->vertexShader);
		glCompileShader(this->fragmentShader);
		// attach shader to Program 
		glAttachShader(this->glslProgram, this->vertexShader);
		glAttachShader(this->glslProgram, this->fragmentShader);
		// Link into ful program, use fixed function vertex pipleline
		glLinkProgram(this->glslProgram);
		GLint linked;
		glGetProgramiv(this->glslProgram, GL_LINK_STATUS, &linked);
		GLint infoLen = 0;
		glGetProgramiv(this->glslProgram, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = new char[infoLen];
			glGetProgramInfoLog(glslProgram, infoLen, NULL, infoLog);
			std::cerr << "Error linking program: \n" << infoLog << std::endl;
			delete[] infoLog;
			exit(-1);
		}
	}
	
	~shaderManager() 
	{
		EGL_CHECK(glDeleteProgram(this->glslProgram));
	}

	inline void useProgram() {
		glUseProgram(this->glslProgram);
	}
};

