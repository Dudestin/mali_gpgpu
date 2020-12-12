#pragma once
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#include <vector>
#include <iostream>

#include "macro.h"

class fboManager
{
	GLuint framebuffer;
	GLuint renderbuffer;
	const GLsizei frameWidth;
	const GLsizei frameHeight;

public:
	fboManager(GLuint frameWidth, GLuint frameHeight)
		: frameWidth(frameWidth), frameHeight(frameHeight)
	{
		glGenFramebuffers(1, &(this->framebuffer));
		glGenRenderbuffers(1, &(this->renderbuffer));
		glBindRenderbuffer(GL_RENDERBUFFER, this->renderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, frameWidth, frameHeight);
		this->bindFBO();
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_RENDERBUFFER, renderbuffer);
		this->clearFBO();
	}

	~fboManager()
	{
		EGL_CHECK(glDeleteFramebuffers(1, &(this->framebuffer)));
		EGL_CHECK(glDeleteFramebuffers(1, &(this->renderbuffer)));
	}

	static GLenum checkCurrentFBOStatus()
	{
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		switch (status) {
		case GL_FRAMEBUFFER_COMPLETE:
			std::cerr << "GL_FRAMEBUFFER_COMPLETE" << std::endl;
			return status;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" << std::endl;
			return status;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS" << std::endl;
			return status;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_FORMATS" << std::endl;
			return status;
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			std::cerr << "GL_FRAMEBUFFER_UNSUPPORTED" << std::endl;
			return status;
			break;
		default:
			std::cerr << "GL_FRAMEBUFFER_IS_NOT_COMPLETE" << std::endl;
			return status;
			break;
		}
	}

	inline void clearFBO() const {
		this->bindFBO();
		glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	inline void bindFBO() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer);
	}

	inline void debindFBO() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	inline void readPixels(GLint x,GLint y, 
		GLsizei width, GLsizei height, GLenum format,
		GLenum type, void* pixels) const
	{
		this->bindFBO();
		glReadPixels(x, y, width, height, format, type, pixels);
	}

	void printPixels() const
	{
		GLubyte* pixels{ new GLubyte[4 * frameWidth, frameHeight] };
		this->readPixels(0, 0, frameWidth, frameHeight,
			GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		for (unsigned int i = 0; i < frameWidth * frameHeight * 4; i += 4) {
			printf("|%p %d %d %d %d|\t", &pixels[i], 
				pixels[i], pixels[i + 1], pixels[i + 2], pixels[i + 3]);
		}
		delete[] pixels;
	}
};