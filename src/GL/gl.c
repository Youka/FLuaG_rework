/*
Project: FLuaG
File: gl.c

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "gl.h"
#include <GLFW/glfw3.h>
#include <limits.h>

PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = 0;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = 0;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = 0;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = 0;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = 0;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = 0;
PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer = 0;

PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = 0;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = 0;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = 0;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample = 0;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv = 0;

PFNGLGENBUFFERSPROC glGenBuffers = 0;
PFNGLBINDBUFFERPROC glBindBuffer = 0;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = 0;
PFNGLBUFFERDATAPROC glBufferData = 0;

PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = 0;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = 0;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = 0;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = 0;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = 0;

PFNGLCREATESHADERPROC glCreateShader = 0;
PFNGLDELETESHADERPROC glDeleteShader = 0;
PFNGLSHADERSOURCEPROC glShaderSource = 0;
PFNGLCOMPILESHADERPROC glCompileShader = 0;
PFNGLGETSHADERIVPROC glGetShaderiv = 0;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = 0;

PFNGLCREATEPROGRAMPROC glCreateProgram = 0;
PFNGLDELETEPROGRAMPROC glDeleteProgram = 0;
PFNGLATTACHSHADERPROC glAttachShader = 0;
PFNGLLINKPROGRAMPROC glLinkProgram = 0;
PFNGLGETPROGRAMIVPROC glGetProgramiv = 0;
PFNGLVALIDATEPROGRAMPROC glValidateProgram = 0;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = 0;
PFNGLUSEPROGRAMPROC glUseProgram = 0;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = 0;
PFNGLUNIFORM1IPROC glUniform1i = 0;
PFNGLUNIFORM1FVPROC glUniform1fv = 0;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = 0;

int glInit(void){
#define GETPROCADDRESS(name, cast) (name = (cast)glfwGetProcAddress(#name))
	return glUniformMatrix4fv || (
			GETPROCADDRESS(glGenFramebuffers, PFNGLGENFRAMEBUFFERSPROC) &&
			GETPROCADDRESS(glBindFramebuffer, PFNGLBINDFRAMEBUFFERPROC) &&
			GETPROCADDRESS(glDeleteFramebuffers, PFNGLDELETEFRAMEBUFFERSPROC) &&
			GETPROCADDRESS(glFramebufferRenderbuffer, PFNGLFRAMEBUFFERRENDERBUFFERPROC) &&
			GETPROCADDRESS(glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2DPROC) &&
			GETPROCADDRESS(glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUSPROC) &&
			GETPROCADDRESS(glBlitFramebuffer, PFNGLBLITFRAMEBUFFERPROC) &&
			GETPROCADDRESS(glGenRenderbuffers, PFNGLGENRENDERBUFFERSPROC) &&
			GETPROCADDRESS(glBindRenderbuffer, PFNGLBINDRENDERBUFFERPROC) &&
			GETPROCADDRESS(glDeleteRenderbuffers, PFNGLDELETERENDERBUFFERSPROC) &&
			GETPROCADDRESS(glRenderbufferStorageMultisample, PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) &&
			GETPROCADDRESS(glGetRenderbufferParameteriv, PFNGLGETRENDERBUFFERPARAMETERIVPROC) &&
			GETPROCADDRESS(glGenBuffers, PFNGLGENBUFFERSPROC) &&
			GETPROCADDRESS(glBindBuffer, PFNGLBINDBUFFERPROC) &&
			GETPROCADDRESS(glDeleteBuffers, PFNGLDELETEBUFFERSPROC) &&
			GETPROCADDRESS(glBufferData, PFNGLBUFFERDATAPROC) &&
			GETPROCADDRESS(glGenVertexArrays, PFNGLGENVERTEXARRAYSPROC) &&
			GETPROCADDRESS(glBindVertexArray, PFNGLBINDVERTEXARRAYPROC) &&
			GETPROCADDRESS(glDeleteVertexArrays, PFNGLDELETEVERTEXARRAYSPROC) &&
			GETPROCADDRESS(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC) &&
			GETPROCADDRESS(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC) &&
			GETPROCADDRESS(glCreateShader, PFNGLCREATESHADERPROC) &&
			GETPROCADDRESS(glDeleteShader, PFNGLDELETESHADERPROC) &&
			GETPROCADDRESS(glShaderSource, PFNGLSHADERSOURCEPROC) &&
			GETPROCADDRESS(glCompileShader, PFNGLCOMPILESHADERPROC) &&
			GETPROCADDRESS(glGetShaderiv, PFNGLGETSHADERIVPROC) &&
			GETPROCADDRESS(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC) &&
			GETPROCADDRESS(glCreateProgram, PFNGLCREATEPROGRAMPROC) &&
			GETPROCADDRESS(glDeleteProgram, PFNGLDELETEPROGRAMPROC) &&
			GETPROCADDRESS(glAttachShader, PFNGLATTACHSHADERPROC) &&
			GETPROCADDRESS(glLinkProgram, PFNGLLINKPROGRAMPROC) &&
			GETPROCADDRESS(glGetProgramiv, PFNGLGETPROGRAMIVPROC) &&
			GETPROCADDRESS(glValidateProgram, PFNGLVALIDATEPROGRAMPROC) &&
			GETPROCADDRESS(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC) &&
			GETPROCADDRESS(glUseProgram, PFNGLUSEPROGRAMPROC) &&
			GETPROCADDRESS(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC) &&
			GETPROCADDRESS(glUniform1i, PFNGLUNIFORM1IPROC) &&
			GETPROCADDRESS(glUniform1fv, PFNGLUNIFORM1FVPROC) &&
			GETPROCADDRESS(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC)
		);
}

GLenum glGetError_s(void){
	/* Get newest error */
	const GLenum error = glGetError();
	/* Clear remaining errors (with limit to prevent endless loop by context errors) */
	if(error != GL_NO_ERROR){
		unsigned short i;
		for(i = 0; glGetError() != GL_NO_ERROR && i < USHRT_MAX; ++i);
	}
	/* Return error code */
	return error;
}

const char* glGetErrorString(const GLenum code){
	switch(code){
		case GL_NO_ERROR: return "GL_NO_ERROR";
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
		case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
		case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
		default: return "GL_UNKNOWN";
	}
}
