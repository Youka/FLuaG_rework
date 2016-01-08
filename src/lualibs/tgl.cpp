/*
Project: FLuaG
File: tgl.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "libs.h"
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <mutex>
#include <vector>

// Unique names for Lua metatables
#define LUA_TGL_CONTEXT "tgl_context"
#define LUA_TGL_SHADER "tgl_shader"
#define LUA_TGL_PROGRAM "tgl_program"
#define LUA_TGL_VAO "tgl_vao"
#define LUA_TGL_TEXTURE "tgl_texture"
#define LUA_TGL_FBO "tgl_fbo"

// GL context management variables for thread-safety
static std::mutex context_mutex;
static unsigned context_count = 0;

// Context metatable methods
static int tgl_context_free(lua_State* L){
	std::unique_lock<std::mutex> lock(context_mutex);
	glfwDestroyWindow(*reinterpret_cast<GLFWwindow**>(luaL_checkudata(L, 1, LUA_TGL_CONTEXT)));
	if(!--context_count)
		glfwTerminate();
	return 0;
}

static int tgl_context_activate(lua_State* L){
	// Set current GL context to use
	glfwMakeContextCurrent(*reinterpret_cast<GLFWwindow**>(luaL_checkudata(L, 1, LUA_TGL_CONTEXT)));
	// Initialize GLEW
	if(glewInit() != GLEW_OK)
		return luaL_error(L, "Couldn't initialize GLEW!");
	// Clear all errors caused by GLFW & GLEW initializations
	glGetError();
	return 0;
}

// Shader metatable methods
static int tgl_shader_free(lua_State* L){
	glDeleteShader(*reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_SHADER)));
	return 0;
}

static int tgl_shader_create(lua_State* L){
	// Get arguments
	const char* shader_type = luaL_checkstring(L, 1),
		*shader_source = luaL_checkstring(L, 2);
	// Create shader
	GLuint shader = glCreateShader(shader_type == std::string("vertex") ? GL_VERTEX_SHADER : (shader_type == std::string("fragment") ? GL_FRAGMENT_SHADER : 0x0));
	if(glGetError() == GL_INVALID_ENUM)
		return luaL_error(L, "Invalid shader type!");
	if(shader == 0)
		return luaL_error(L, "Couldn't generate shader!");
	// Set shader source
	glShaderSource(shader, 1, &shader_source, nullptr);
	// Compile shader
	glCompileShader(shader);
	GLint shader_status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_status);
	if(shader_status == GL_FALSE){
		GLint shader_info_length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &shader_info_length);
		std::string shader_info(shader_info_length, '\0');
		glGetShaderInfoLog(shader, shader_info_length, &shader_info_length, const_cast<char*>(shader_info.data()));
		glDeleteShader(shader);
		return luaL_error(L, shader_info.c_str());
	}
	// Create userdata for GL shader
	*reinterpret_cast<GLuint*>(lua_newuserdata(L, sizeof(GLuint))) = shader;
	// Fetch/create Lua tgl shader metatable
	if(luaL_newmetatable(L, LUA_TGL_SHADER)){
		lua_pushcfunction(L, tgl_shader_free); lua_setfield(L, -2, "__gc");
	}
	// Bind metatable to userdata
	lua_setmetatable(L, -2);
	// Return the userdata to Lua
	return 1;
}

// Program metatable methods
static int tgl_program_free(lua_State* L){
	glDeleteProgram(*reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_PROGRAM)));
	return 0;
}

static int tgl_program_use(lua_State* L){
	glUseProgram(*reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_PROGRAM)));
        if(glGetError() == GL_INVALID_OPERATION)
		return luaL_error(L, "Can't use this program!");
	return 0;
}

static int tgl_program_uniform(lua_State* L){
	// Get main arguments
	GLuint program = *reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_PROGRAM));
	const char* location = luaL_checkstring(L, 2);
	const std::string data_type = luaL_checkstring(L, 3);
	// Get number of further arguments
	const int argc = lua_gettop(L) - 3;
	if(argc == 0)
		return luaL_error(L, "Not enough arguments!");
	// Get shader location
	const GLint location_index = glGetUniformLocation(program, location);
	if(location_index == -1)
		return luaL_error(L, "Couldn't detect location!");
	// Choose operation by data type
	if(data_type == "f")
		switch(argc){
			case 1: glUniform1f(location_index, luaL_checknumber(L, 4)); break;
			case 2: glUniform2f(location_index, luaL_checknumber(L, 4), luaL_checknumber(L, 5)); break;
			case 3: glUniform3f(location_index, luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6)); break;
			default: glUniform4f(location_index, luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6), luaL_checknumber(L, 7)); break;
		}
	else if(data_type == "i")
		switch(argc){
			case 1: glUniform1i(location_index, luaL_checkinteger(L, 4)); break;
			case 2: glUniform2i(location_index, luaL_checkinteger(L, 4), luaL_checkinteger(L, 5)); break;
			case 3: glUniform3i(location_index, luaL_checkinteger(L, 4), luaL_checkinteger(L, 5), luaL_checkinteger(L, 6)); break;
			default: glUniform4i(location_index, luaL_checkinteger(L, 4), luaL_checkinteger(L, 5), luaL_checkinteger(L, 6), luaL_checkinteger(L, 7)); break;
		}
	else if(data_type == "1f" || data_type == "2f" || data_type == "3f" || data_type == "4f"){
		std::vector<float> values(argc);
		for(int i = 0; i < argc; ++i)
			values[i] = luaL_checknumber(L, 4+i);
		switch(data_type.front() - '0'){
			case 1:
				glUniform1fv(location_index, values.size(), values.data());
				break;
			case 2:
				if(values.size() & 0x1)
					return luaL_error(L, "Number of arguments doesn't meet requirements!");
				glUniform2fv(location_index, values.size() << 1, values.data());
				break;
			case 3:
				if(values.size() % 3)
					return luaL_error(L, "Number of arguments doesn't meet requirements!");
				glUniform3fv(location_index, values.size() / 3, values.data());
				break;
			case 4:
				if(values.size() & 0x3)
					return luaL_error(L, "Number of arguments doesn't meet requirements!");
				glUniform4fv(location_index, values.size() << 2, values.data());
				break;
		}
	}else if(data_type == "1i" || data_type == "2i" || data_type == "3i" || data_type == "4i"){
		std::vector<int> values(argc);
		for(int i = 0; i < argc; ++i)
			values[i] = luaL_checkinteger(L, 4+i);
		switch(data_type.front() - '0'){
			case 1:
				glUniform1iv(location_index, values.size(), values.data());
				break;
			case 2:
				if(values.size() & 0x1)
					return luaL_error(L, "Number of arguments doesn't meet requirements!");
				glUniform2iv(location_index, values.size() << 1, values.data());
				break;
			case 3:
				if(values.size() % 3)
					return luaL_error(L, "Number of arguments doesn't meet requirements!");
				glUniform3iv(location_index, values.size() / 3, values.data());
				break;
			case 4:
				if(values.size() & 0x3)
					return luaL_error(L, "Number of arguments doesn't meet requirements!");
				glUniform4iv(location_index, values.size() << 2, values.data());
				break;
		}
	}else if(data_type == "mat"){
		std::vector<float> values(argc);
		for(int i = 0; i < argc; ++i)
			values[i] = luaL_checknumber(L, 4+i);
		switch(values.size()){
			case 4:
				glUniformMatrix2fv(location_index, 1, 0, values.data());
				break;
			case 9:
				glUniformMatrix3fv(location_index, 1, 0, values.data());
				break;
			case 16:
				glUniformMatrix4fv(location_index, 1, 0, values.data());
				break;
			default:
				return luaL_error(L, "Number of arguments doesn't meet any requirements!");
		}
	}
	// Check for error
	if(glGetError() == GL_INVALID_OPERATION)
		return luaL_error(L, "Setting uniform failed!");
	return 0;
}

static int tgl_program_create(lua_State* L){
	// Get arguments
	GLuint vshader = *reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_SHADER)),
		fshader = *reinterpret_cast<GLuint*>(luaL_checkudata(L, 2, LUA_TGL_SHADER));
	// Check arguments
	GLint vshader_type, fshader_type;
	glGetShaderiv(vshader, GL_SHADER_TYPE, &vshader_type);
	glGetShaderiv(fshader, GL_SHADER_TYPE, &fshader_type);
	if(vshader_type != GL_VERTEX_SHADER || fshader_type != GL_FRAGMENT_SHADER)
		return luaL_error(L, "Vertex & fragment shader expected!");
	// Create program
	GLuint program = glCreateProgram();
	if(program == 0)
		return luaL_error(L, "Couldn't generate program!");
	// Attach shaders to program
        glAttachShader(program, vshader);
        glAttachShader(program, fshader);
	// Link & complete program
	glLinkProgram(program);
	GLint program_status;
	glGetProgramiv(program, GL_LINK_STATUS, &program_status);
	if(program_status == GL_FALSE){
		GLint program_info_length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &program_info_length);
		std::string program_info(program_info_length, '\0');
		glGetProgramInfoLog(program, program_info_length, &program_info_length, const_cast<char*>(program_info.data()));
		glDeleteProgram(program);
		return luaL_error(L, program_info.c_str());
	}
	// Create userdata for GL program
	*reinterpret_cast<GLuint*>(lua_newuserdata(L, sizeof(GLuint))) = program;
	// Fetch/create Lua tgl program metatable
	if(luaL_newmetatable(L, LUA_TGL_PROGRAM)){
		lua_pushcfunction(L, tgl_program_free); lua_setfield(L, -2, "__gc");
		lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, tgl_program_use); lua_setfield(L, -2, "use");
		lua_pushcfunction(L, tgl_program_uniform); lua_setfield(L, -2, "uniform");
	}
	// Bind metatable to userdata
	lua_setmetatable(L, -2);
	// Return the userdata to Lua
	return 1;
}

// VAO metatable methods
static int tgl_vao_create(lua_State* L){

	// TODO

	return 0;
}

int luaopen_tgl(lua_State* L){
	// Thread-lock for safe GLFW usage
	std::unique_lock<std::mutex> lock(context_mutex);
	// Initialize GLFW with general properties
	if(!context_count){
		if(glfwInit() != GL_TRUE)
			return luaL_error(L, "Couldn't initialize GLFW!");
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
		glfwWindowHint(GLFW_DECORATED, GL_FALSE);
		glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	}
	// Create GL context
	GLFWwindow* window = glfwCreateWindow(1, 1, "tgl", nullptr, nullptr);
	if(!window)
		return luaL_error(L, "Couldn't create GL context!");
	// Create userdata for GL context
	GLFWwindow** udata = reinterpret_cast<GLFWwindow**>(lua_newuserdata(L, sizeof(GLFWwindow*)));
	*udata = window;
	// Fetch/create Lua tgl context metatable
	if(luaL_newmetatable(L, LUA_TGL_CONTEXT)){
		lua_pushcfunction(L, tgl_context_free); lua_setfield(L, -2, "__gc");
		lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, tgl_context_activate); lua_setfield(L, -2, "activate");
		lua_pushcfunction(L, tgl_shader_create); lua_setfield(L, -2, "createshader");
		lua_pushcfunction(L, tgl_program_create); lua_setfield(L, -2, "createprogram");
		lua_pushcfunction(L, tgl_vao_create); lua_setfield(L, -2, "createvao");
	}
	// Bind metatable to userdata
	lua_setmetatable(L, -2);
	// Increment counter for finally created context
	++context_count;
	// Return the userdata to Lua
	return 1;
}
