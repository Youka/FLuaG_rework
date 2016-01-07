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
	return 0;
}

// Shader metatable methods
static int tgl_shader_create(lua_State* L){
	const char* shader_type = luaL_checkstring(L, 1),
		*shader_source = luaL_checkstring(L, 2);
	GLuint shader = glCreateShader(GL_VERTEX_SHADER);

	// TODO

	*reinterpret_cast<GLuint*>(lua_newuserdata(L, sizeof(GLuint))) = shader;
	if(luaL_newmetatable(L, LUA_TGL_SHADER)){

		// TODO

	}
	lua_setmetatable(L, -2);
	return 1;
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
	}
	// Bind metatable to userdata
	lua_setmetatable(L, -2);
	// Increment counter for finally created context
	++context_count;
	// Return the userdata to Lua
	return 1;
}
