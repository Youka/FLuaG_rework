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
#include "../utils/lua.h"
#define GLEW_STATIC
#include <glew.h>
#include <glfw3.h>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cassert>

// OpenGL error check (application hang without context)
static bool glHasError(){
	// No errors?
	if(glGetError() == GL_NO_ERROR)
		return false;
	// Clear remaining errors
	while(glGetError() != GL_NO_ERROR);
	// Had error(s)!
	return true;
}

// Unique names for Lua metatables
#define LUA_TGL_CONTEXT "tgl_context"
#define LUA_TGL_SHADER "tgl_shader"
#define LUA_TGL_PROGRAM "tgl_program"
#define LUA_TGL_VAO "tgl_vao"
#define LUA_TGL_TEXTURE "tgl_texture"
#define LUA_TGL_FBO "tgl_fbo"

// GL context management variables
static std::mutex context_mutex;
static unsigned context_count = 0;

// OpenGL context check in Lua (to insert on function begin)
#define TGL_CHECK_CONTEXT if(!glfwGetCurrentContext()) return luaL_error(L, "No context!");

// Context metatable methods
static int tgl_context_free(lua_State* L){
	std::unique_lock<std::mutex> context_lock(context_mutex);
	glfwDestroyWindow(*static_cast<GLFWwindow**>(luaL_checkudata(L, 1, LUA_TGL_CONTEXT)));
	if(!--context_count)
		glfwTerminate();
	return 0;
}

static int tgl_context_activate(lua_State* L){
	// Set current GL context to use
	glfwMakeContextCurrent(lua_isnoneornil(L, 1) ? nullptr :  *static_cast<GLFWwindow**>(luaL_checkudata(L, 1, LUA_TGL_CONTEXT)));
	// Further context preparations
	if(glfwGetCurrentContext()){
		// Initialize GLEW
		std::unique_lock<std::mutex> context_lock(context_mutex);
		glewExperimental = GL_TRUE;
		if(glewInit() != GLEW_OK)
			return luaL_error(L, "Couldn't initialize GLEW!");
		// Clear all errors caused by GLFW & GLEW initializations
		glHasError();
	}
	return 0;
}

// Shader metatable methods
static int tgl_shader_free(lua_State* L){
	glDeleteShader(*static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_SHADER)));
	return 0;
}

static int tgl_shader_create(lua_State* L){
	TGL_CHECK_CONTEXT
	// Get arguments
	static const char* option_str[] = {"vertex", "fragment", nullptr};
	static const GLenum option_enum[] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
	const GLenum shader_type = option_enum[luaL_checkoption(L, 1, nullptr, option_str)];
	const char* shader_source = luaL_checkstring(L, 2);
	// Create shader
	const GLuint shader = glCreateShader(shader_type);
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
	*static_cast<GLuint*>(lua_newuserdata(L, sizeof(GLuint))) = shader;
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
	glDeleteProgram(*static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_PROGRAM)));
	return 0;
}

static int tgl_program_use(lua_State* L){
	TGL_CHECK_CONTEXT
	glUseProgram(*static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_PROGRAM)));
	if(glHasError())
		return luaL_error(L, "Can't use this program!");
	return 0;
}

static int tgl_program_uniform(lua_State* L){
	TGL_CHECK_CONTEXT
	// Get main arguments
	const GLuint program = *static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_PROGRAM));
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
	if(glHasError())
		return luaL_error(L, "Setting uniform failed!");
	return 0;
}

static int tgl_program_create(lua_State* L){
	TGL_CHECK_CONTEXT
	// Get arguments
	const GLuint vshader = *static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_SHADER)),
		fshader = *static_cast<GLuint*>(luaL_checkudata(L, 2, LUA_TGL_SHADER));
	// Check arguments
	GLint vshader_type, fshader_type;
	glGetShaderiv(vshader, GL_SHADER_TYPE, &vshader_type);
	glGetShaderiv(fshader, GL_SHADER_TYPE, &fshader_type);
	if(vshader_type != GL_VERTEX_SHADER || fshader_type != GL_FRAGMENT_SHADER)
		return luaL_error(L, "Vertex & fragment shader expected!");
	// Create program
	const GLuint program = glCreateProgram();
	if(program == 0)
		return luaL_error(L, "Couldn't generate program!");
	// Attach shaders to program
        glAttachShader(program, vshader);
        glAttachShader(program, fshader);
	// Link & complete program
	glLinkProgram(program);
	GLint program_status;
	glGetProgramiv(program, GL_LINK_STATUS, &program_status);
	if(program_status == GL_FALSE || (glValidateProgram(program), glGetProgramiv(program, GL_VALIDATE_STATUS, &program_status), program_status) == GL_FALSE){
		GLint program_info_length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &program_info_length);
		std::string program_info(program_info_length, '\0');
		glGetProgramInfoLog(program, program_info_length, &program_info_length, const_cast<char*>(program_info.data()));
		glDeleteProgram(program);
		return luaL_error(L, program_info.c_str());
	}
	// Create userdata for GL program
	*static_cast<GLuint*>(lua_newuserdata(L, sizeof(GLuint))) = program;
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
static int tgl_vao_free(lua_State* L){
	const GLuint* udata = static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_VAO));
	glDeleteVertexArrays(1, &udata[1]);
	glDeleteBuffers(1, &udata[0]);
	return 0;
}

static int tgl_vao_draw(lua_State* L){
	TGL_CHECK_CONTEXT
	// Get arguments
	const GLuint* udata = static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_VAO));
	static const char* option_str[] = {"points", "line strip", "line loop", "lines", "triangle strip", "triangle fan", "triangles", nullptr};
	static const GLenum option_enum[] = {GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES};
	const GLenum mode = option_enum[luaL_checkoption(L, 2, nullptr, option_str)];
	const int first = luaL_checkinteger(L, 3);
	const GLsizei count = luaL_checkinteger(L, 4);
	// Check arguments
	if(first < 0 || count <= 0)
		return luaL_error(L, "Invalid first or count!");
	// Bind VAO for data access
	glBindVertexArray(udata[1]);
	// Draw (send vertex data to shader)
	glDrawArrays(mode, first, count);
	if(glHasError())
		return luaL_error(L, "Drawing operation was invalid!");
	return 0;
}

static int tgl_vao_create(lua_State* L){
	TGL_CHECK_CONTEXT
	// Check arguments
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checktype(L, 2, LUA_TTABLE);
	// Get properties
	struct Property{
		int location_index, vertex_size;
	};
	std::vector<Property> props(lua_rawlen(L, 1));
	for(size_t prop_i = 1; prop_i <= props.size(); ++prop_i){
		lua_rawgeti(L, 1, prop_i);
		if(lua_istable(L, -1)){
			lua_getfield(L, -1, "location");
			lua_getfield(L, -2, "size");
			if(!lua_isnumber(L, -2) || !lua_isnumber(L, -1))
				return luaL_error(L, "Properties 'location' and 'size' have to be numbers!");
			props[prop_i-1] = {lua_tointeger(L, -2), lua_tointeger(L, -1)};
			lua_pop(L, 2);
		}else
			return luaL_error(L, "Properties have to be tables!");
		lua_pop(L, 1);
	}
	// Convert data to vector
	std::vector<float> data(lua_rawlen(L, 2));
	for(size_t i = 1; i <= data.size(); ++i){
		lua_rawgeti(L, 2, i);
		if(!lua_isnumber(L, -1))
			return luaL_error(L, "Data must contain numbers only!");
		data[i-1] = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}
        // Create VBO
        GLuint vbo;
        glGenBuffers(1, &vbo);
        // Fill VBO
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() << 2, data.data(), GL_STATIC_DRAW);
        if(glHasError()){
		glDeleteBuffers(1, &vbo);
		return luaL_error(L, "Couldn't allocate memory for VBO data!");
        }
        // Create VAO
	GLuint vao;
	glGenVertexArrays(1, &vao);
	// Configure VAO
	glBindVertexArray(vao);
	const GLsizei stride = std::accumulate(props.cbegin(), props.cend(), 0, [](const int accum, const Property& prop){return accum + prop.vertex_size;}) << 2;
	GLbyte* offset = 0;
	for(const auto& prop : props){
		glEnableVertexAttribArray(prop.location_index);
		if(glHasError()){
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, &vbo);
                        return luaL_error(L, "Invalid location!");
		}
		glVertexAttribPointer(prop.location_index, prop.vertex_size, GL_FLOAT, GL_FALSE, stride, offset);
		if(glHasError()){
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, &vbo);
                        return luaL_error(L, "Invalid vertex size!");
		}
		offset += prop.vertex_size << 2;
	}
	// Create userdata for VAO (+VBO)
	GLuint* udata = static_cast<GLuint*>(lua_newuserdata(L, sizeof(GLuint) << 1));
	udata[0] = vbo;
	udata[1] = vao;
	// Fetch/create Lua tgl vao metatable
	if(luaL_newmetatable(L, LUA_TGL_VAO)){
		lua_pushcfunction(L, tgl_vao_free); lua_setfield(L, -2, "__gc");
		lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, tgl_vao_draw); lua_setfield(L, -2, "draw");
	}
	// Bind metatable to userdata
	lua_setmetatable(L, -2);
	// Return the userdata to Lua
	return 1;
}

// Texture metatable methods
static int tgl_texture_free(lua_State* L){
	const GLuint* udata = static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_TEXTURE));
	glDeleteTextures(1, &udata[0]);
	if(udata[1])
		glDeleteBuffers(1, &udata[1]);
	return 0;
}

static int tgl_texture_param(lua_State* L){
	TGL_CHECK_CONTEXT
	// Get arguments
	const GLuint tex = *static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_TEXTURE));
	const std::string param = luaL_checkstring(L, 2);
	// Bind texture for access
	glBindTexture(GL_TEXTURE_2D, tex);
	// Set texture parameters
	if(param == "filter"){
		const std::string value = luaL_checkstring(L, 3);
		if(value == "nearest"){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}else if(value == "linear"){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}else
			return luaL_error(L, "Invalid filter value!");
	}else if(param == "wrap"){
		const std::string value = luaL_checkstring(L, 3);
		if(value == "clamp"){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}else if(value == "repeat"){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}else if(value == "mirror"){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		}else
			return luaL_error(L, "Invalid wrap value!");
	}else if(param == "mipmaps"){
		glGenerateMipmap(GL_TEXTURE_2D);
	}else
		return luaL_error(L, "Invalid parameter!");
	return 0;
}

static int tgl_texture_data(lua_State* L){
	TGL_CHECK_CONTEXT
	// Get arguments
	GLuint* udata = static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_TEXTURE));
	static const char* option_str[] = {"rgb", "bgr", "rgba", "bgra", "none", nullptr};
	static const GLenum option_enum[] = {GL_RGB, GL_BGR, GL_RGBA, GL_BGRA, 0x0};
	const GLenum request_format = option_enum[luaL_checkoption(L, 2, "none", option_str)];
	// Bind texture for access
	glBindTexture(GL_TEXTURE_2D, udata[0]);
	// Get texture header
	GLint width, height, format;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
	// Push texture header to Lua
	lua_pushnumber(L, width);
	lua_pushnumber(L, height);
	switch(format){
		case GL_RGB: lua_pushstring(L, "rgb"); break;
		case GL_RGBA: lua_pushstring(L, "rgba"); break;
		default: lua_pushnumber(L, format);	// Should never happen
	}
	// Get+push optional texture data & return to Lua
	if(request_format){
		// Calculate data size
		const size_t data_size = width * height * (request_format == GL_RGB || request_format == GL_BGR ? 3 : 4);
		// Create/bind PBO
		if(udata[1]){
			glBindBuffer(GL_PIXEL_PACK_BUFFER, udata[1]);
			GLint buffer_size;
			glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER, GL_BUFFER_SIZE, &buffer_size);
			if(static_cast<size_t>(buffer_size) < data_size){
				glDeleteBuffers(1, &udata[1]);
				udata[1] = 0;
			}
		}
		if(!udata[1]){
			GLuint pbo;
			glGenBuffers(1, &pbo);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
			glBufferData(GL_PIXEL_PACK_BUFFER, data_size, nullptr, GL_STREAM_READ);	// Reserve enough memory for all supported formats
			if(glHasError()){
				glDeleteBuffers(1, &pbo);
				return luaL_error(L, "Couldn't allocate memory for PBO!");
			}
			udata[1] = pbo;
		}
		// Read texture to PBO
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glGetTexImage(GL_TEXTURE_2D, 0, request_format, GL_UNSIGNED_BYTE, nullptr);
		// Copy/push PBO to Lua
		GLvoid* pbo_map = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		if(glHasError() || !pbo_map)
			return luaL_error(L, "Couldn't allocate virtual memory for PBO mapping!");
		lua_pushlstring(L, static_cast<char*>(pbo_map), data_size);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		// Return header + data to Lua
		return 4;
	}
	// Return only the header to Lua
	return 3;
}

static int tgl_texture_bind(lua_State* L){
	TGL_CHECK_CONTEXT
	// Set active texture
	glActiveTexture(GL_TEXTURE0 + luaL_optinteger(L, 2, 0));
	if(glHasError())
		return luaL_error(L, "Invalid unit!");
	// Bind texture
	glBindTexture(GL_TEXTURE_2D, *static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_TEXTURE)));
	return 0;
}

static int tgl_texture_create(lua_State* L){
	TGL_CHECK_CONTEXT
	// Get arguments
	const GLsizei width = luaL_checkinteger(L, 1),
		height = luaL_checkinteger(L, 2);
	static const char* option_str[] = {"rgb", "bgr", "rgba", "bgra", nullptr};
	static const GLenum option_enum[] = {GL_RGB, GL_BGR, GL_RGBA, GL_BGRA};
	const GLenum format = option_enum[luaL_checkoption(L, 3, nullptr, option_str)];
	size_t data_len;
	const char* data = luaL_optlstring(L, 4, nullptr, &data_len);
	// Check arguments
	if(width <= 0 || height <= 0)
		return luaL_error(L, "Invalid dimensions!");
	if(data && data_len != static_cast<size_t>(width * height * (format == GL_RGB || format == GL_BGR ? 3 : 4)))
		return luaL_error(L, "Data size doesn't fit!");
	// Create texture
	GLuint tex;
	glGenTextures(1, &tex);
	// Fill texture
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, format == GL_BGR ? GL_RGB : (format == GL_BGRA ? GL_RGBA : format), width, height, 0, format,  GL_UNSIGNED_BYTE, data);
	if(glHasError()){
		glDeleteTextures(1, &tex);
		return luaL_error(L, "Invalid texture value!");
	}
	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// Create userdata for texture
	GLuint* udata = static_cast<GLuint*>(lua_newuserdata(L, sizeof(GLuint) << 1));
	udata[0] = tex;
	udata[1] = 0;	// Reserved for PBO on data access
	// Fetch/create Lua tgl texture metatable
	if(luaL_newmetatable(L, LUA_TGL_TEXTURE)){
		lua_pushcfunction(L, tgl_texture_free); lua_setfield(L, -2, "__gc");
		lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, tgl_texture_param); lua_setfield(L, -2, "parameter");
		lua_pushcfunction(L, tgl_texture_data); lua_setfield(L, -2, "data");
		lua_pushcfunction(L, tgl_texture_bind); lua_setfield(L, -2, "bind");
	}
	// Bind metatable to userdata
	lua_setmetatable(L, -2);
	// Return the userdata to Lua
	return 1;
}

// Framebuffer metatable methods
static int tgl_fbo_free(lua_State* L){
	const GLuint* udata = static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_FBO));
	glDeleteFramebuffers(1, &udata[2]);
	glDeleteRenderbuffers(2, &udata[0]);
	return 0;
}

static int tgl_fbo_bind(lua_State *L){
	glBindFramebuffer(GL_FRAMEBUFFER, *static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_FBO)));
	return 0;
}

static int tgl_fbo_info(lua_State *L){
	// Temporary bind RBO and get data
	glBindRenderbuffer(GL_RENDERBUFFER, *static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_FBO)));
	GLint width, height, samples;
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);
	// Send collected data to Lua
	lua_pushnumber(L, width);
	lua_pushnumber(L, height);
	lua_pushnumber(L, samples);
	return 3;
}

static int tgl_fbo_blit(lua_State *L){
	// Get arguments
	const GLuint* udata = static_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_FBO));
	const GLuint tex = *static_cast<GLuint*>(luaL_checkudata(L, 2, LUA_TGL_TEXTURE));
	// Get source FBO dimensions
	glBindRenderbuffer(GL_RENDERBUFFER, udata[0]);
	GLint width, height;
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
	// Generate & bind destination FBO
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	// Bind source FBO
	glBindFramebuffer(GL_READ_FRAMEBUFFER, udata[2]);
	// Blit source to destination FBO (includes downsampling)
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	if(glHasError()){
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &fbo);
		return luaL_error(L, "Couldn't copy framebuffer to texture data!");
	}
	// Reset FBOs to default
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Delete no longer needed FBO
	glDeleteFramebuffers(1, &fbo);
	// Return input texture
	return 1;
}

static int tgl_fbo_create(lua_State* L){
	TGL_CHECK_CONTEXT
	// Get arguments
	const int width = luaL_checkinteger(L, 1),
		height = luaL_checkinteger(L, 2),
		samples = luaL_optinteger(L, 3, 0);
	// Generate RBOs
	GLuint rbo[2];
	glGenRenderbuffers(2, rbo);
	// Configure RBOs
	glBindRenderbuffer(GL_RENDERBUFFER, rbo[0]);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo[1]);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
	if(glHasError()){
		glDeleteRenderbuffers(2, rbo);
		return luaL_error(L, "Couldn't allocate memory for FBO buffers!");
	}
	// Generate FBO
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	// Bind RBOS to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo[0]);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);
	// Check FBO completeness
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &fbo);
		glDeleteRenderbuffers(2, rbo);
		return luaL_error(L, "Framebuffer couldn't get completed!");
	}
	// Reset FBO to default
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Create userdata for FBO
	GLuint* udata = static_cast<GLuint*>(lua_newuserdata(L, sizeof(GLuint) * 3));
	udata[0] = rbo[0];
	udata[1] = rbo[1];
	udata[2] = fbo;
	// Fetch/create Lua tgl fbo metatable
	if(luaL_newmetatable(L, LUA_TGL_FBO)){
		lua_pushcfunction(L, tgl_fbo_free); lua_setfield(L, -2, "__gc");
		lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, tgl_fbo_bind); lua_setfield(L, -2, "bind");
		lua_pushcfunction(L, tgl_fbo_info); lua_setfield(L, -2, "info");
		lua_pushcfunction(L, tgl_fbo_blit); lua_setfield(L, -2, "blit");
	}
	// Bind metatable to userdata
	lua_setmetatable(L, -2);
	// Return the userdata to Lua
	return 1;
}

// General functions
static int tgl_clear(lua_State* L){
	TGL_CHECK_CONTEXT
	// Get main argument
	static const char* option_str[] = {"color", "depth", "stencil", nullptr};
	static const GLenum option_enum[] = {GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT, GL_STENCIL_BUFFER_BIT};
	const GLenum buffer = option_enum[luaL_checkoption(L, 1, nullptr, option_str)];
	// Progress by buffer type
	switch(buffer){
		case GL_COLOR_BUFFER_BIT:
			glClearColor(luaL_optnumber(L, 2, 0), luaL_optnumber(L, 3, 0), luaL_optnumber(L, 4, 0), luaL_optnumber(L, 5, 0));
			break;
		case GL_DEPTH_BUFFER_BIT:
			glClearDepth(luaL_optnumber(L, 2, 1));
			break;
		case GL_STENCIL_BUFFER_BIT:
			glClearStencil(luaL_optinteger(L, 2, 0));
			break;
	}
	glClear(buffer);
	return 0;
}

static int tgl_viewport(lua_State* L){
	TGL_CHECK_CONTEXT
	glViewport(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4));
        if(glHasError())
		return luaL_error(L, "Invalid rectangle!");
	return 0;
}

static int tgl_size(lua_State* L){
	TGL_CHECK_CONTEXT
	static const char* option_str[] = {"points", "lines", nullptr};
	static const GLenum option_enum[] = {GL_POINTS, GL_LINES};
	switch(option_enum[luaL_checkoption(L, 1, nullptr, option_str)]){
		case GL_POINTS:
			glPointSize(luaL_checknumber(L, 2));
			break;
		case GL_LINES:
			glLineWidth(luaL_checknumber(L, 2));
			break;
	}
        if(glHasError())
		return luaL_error(L, "Size have to be greater than zero!");
	return 0;
}

static int tgl_mode(lua_State* L){
	TGL_CHECK_CONTEXT
	static const char* option_str[] = {"point", "line", "fill", nullptr};
	static const GLenum option_enum[] = {GL_POINT, GL_LINE, GL_FILL};
	glPolygonMode(GL_FRONT_AND_BACK, option_enum[luaL_checkoption(L, 1, nullptr, option_str)]);
	return 0;
}

static int tgl_scissor(lua_State* L){
	TGL_CHECK_CONTEXT
	if(lua_gettop(L)){
		glScissor(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4));
		if(glHasError())
			return luaL_error(L, "Invalid rectangle!");
		glEnable(GL_SCISSOR_TEST);
	}else
		glDisable(GL_SCISSOR_TEST);
	return 0;
}

static int tgl_logic(lua_State* L){
	TGL_CHECK_CONTEXT
	if(lua_gettop(L)){
		static const char* option_str[] = {"clear", "set", "copy", "copy inverted", "noop", "invert", "and", "nand", "or", "nor", "xor", "equiv", "and reverse", "and inverted", "or reverse", "or inverted", nullptr};
		static const GLenum option_enum[] = {GL_CLEAR, GL_SET, GL_COPY, GL_COPY_INVERTED, GL_NOOP, GL_INVERT, GL_AND, GL_NAND, GL_OR, GL_NOR, GL_XOR, GL_EQUIV, GL_AND_REVERSE, GL_AND_INVERTED, GL_OR_REVERSE, GL_OR_INVERTED};
		glLogicOp(option_enum[luaL_checkoption(L, 1, nullptr, option_str)]);
		glEnable(GL_COLOR_LOGIC_OP);
	}else
		glDisable(GL_COLOR_LOGIC_OP);
	return 0;
}

static int tgl_mask(lua_State* L){
	TGL_CHECK_CONTEXT
	static const char* option_str[] = {"color", "depth", "stencil", nullptr};
	static const GLenum option_enum[] = {GL_COLOR, GL_DEPTH, GL_STENCIL};
	switch(option_enum[luaL_checkoption(L, 1, nullptr, option_str)]){
		case GL_COLOR:
			glColorMask(luaL_checkboolean(L, 2), luaL_checkboolean(L, 3), luaL_checkboolean(L, 4), luaL_checkboolean(L, 5));
			break;
		case GL_DEPTH:
			glDepthMask(luaL_checkboolean(L, 2));
			break;
		case GL_STENCIL:
			glStencilMask(luaL_checkinteger(L, 2));
			break;
	}
	return 0;
}

static int tgl_depth(lua_State* L){
	TGL_CHECK_CONTEXT
	if(lua_gettop(L)){
		static const char* option_str[] = {"never", "less", "equal", "less equal", "greater", "not equal", "greater equal", "always", nullptr};
		static const GLenum option_enum[] = {GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS};
		glDepthFunc(option_enum[luaL_checkoption(L, 1, nullptr, option_str)]);
		glEnable(GL_DEPTH_TEST);
	}else
		glDisable(GL_DEPTH_TEST);
	return 0;
}

static int tgl_stencil(lua_State* L){
	TGL_CHECK_CONTEXT
	if(lua_gettop(L)){
		// Check argument
		luaL_checktype(L, 1, LUA_TTABLE);
		// Set stencil function
		lua_getfield(L, 1, "test_function");
		lua_getfield(L, 1, "test_reference");
		lua_getfield(L, 1, "test_mask");
		if(!(lua_isnil(L, -3) && lua_isnil(L, -2) && lua_isnil(L, -1))){
			static const char* option_str[] = {"never", "less", "less equal", "greater", "greater equal", "equal", "not equal", "always", nullptr};
			static const GLenum option_enum[] = {GL_NEVER, GL_LESS, GL_LEQUAL, GL_GREATER, GL_GEQUAL, GL_EQUAL, GL_NOTEQUAL, GL_ALWAYS};
			GLenum func;
			GLint ref, mask;
			if(lua_isnil(L, -3))
				glGetIntegerv(GL_STENCIL_FUNC, reinterpret_cast<GLint*>(&func));
			else
				func = option_enum[luaL_checkoption(L, -3, nullptr, option_str)];
			if(lua_isnil(L, -2))
				glGetIntegerv(GL_STENCIL_REF, &ref);
			else
				ref = luaL_checkinteger(L, -2);
			if(lua_isnil(L, -1))
				glGetIntegerv(GL_STENCIL_VALUE_MASK, &mask);
			else
				mask = luaL_checkinteger(L, -1);
			glStencilFunc(func, ref, mask);
		}
		lua_pop(L, 3);
		// Set stencil operation
		lua_getfield(L, 1, "stencil_fail");
		lua_getfield(L, 1, "depth_fail");
		lua_getfield(L, 1, "stencil_depth_pass");
		if(!(lua_isnil(L, -3) && lua_isnil(L, -2) && lua_isnil(L, -1))){
			static const char* op_option_str[] = {"keep", "zero", "replace", "increment", "increment wrap", "decrement", "decrement wrap", "invert", nullptr};
			static const GLenum op_option_enum[] = {GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_INCR_WRAP, GL_DECR, GL_DECR_WRAP, GL_INVERT};
			GLenum sfail, dfail, sdpass;
			if(lua_isnil(L, -3))
				glGetIntegerv(GL_STENCIL_FAIL, reinterpret_cast<GLint*>(&sfail));
			else
				sfail = op_option_enum[luaL_checkoption(L, -3, nullptr, op_option_str)];
			if(lua_isnil(L, -2))
				glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, reinterpret_cast<GLint*>(&dfail));
			else
				dfail = op_option_enum[luaL_checkoption(L, -2, nullptr, op_option_str)];
			if(lua_isnil(L, -1))
				glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, reinterpret_cast<GLint*>(&sdpass));
			else
				sdpass = op_option_enum[luaL_checkoption(L, -1, nullptr, op_option_str)];
			glStencilOp(sfail, dfail, sdpass);
		}
		lua_pop(L, 3);
		// Enable stenciling
		glEnable(GL_STENCIL_TEST);
	}else
		glDisable(GL_STENCIL_TEST);
	return 0;
}

static int tgl_blend(lua_State* L){
	TGL_CHECK_CONTEXT
	if(lua_gettop(L)){
		// Check argument
		luaL_checktype(L, 1, LUA_TTABLE);
		// Set blend color
		lua_getfield(L, 1, "constant_red");
		lua_getfield(L, 1, "constant_green");
		lua_getfield(L, 1, "constant_blue");
		lua_getfield(L, 1, "constant_alpha");
		if(!(lua_isnil(L, -4) && lua_isnil(L, -3) && lua_isnil(L, -2) && lua_isnil(L, -1))){
			GLclampf color[4];
			glGetFloatv(GL_BLEND_COLOR, color);
			if(!lua_isnil(L, -4))
				color[0] = luaL_checknumber(L, -4);
			if(!lua_isnil(L, -3))
				color[1] = luaL_checknumber(L, -3);
			if(!lua_isnil(L, -2))
				color[2] = luaL_checknumber(L, -2);
			if(!lua_isnil(L, -1))
				color[3] = luaL_checknumber(L, -1);
			glBlendColor(color[0], color[1], color[2], color[3]);
		}
		lua_pop(L, 4);
		// Set blend equation
		lua_getfield(L, 1, "rgb_equation");
		lua_getfield(L, 1, "alpha_equation");
		if(!(lua_isnil(L, -1) && lua_isnil(L, -2))){
			static const char* option_str[] = {"add", "subtract", "reverse subtract", "min", "max", nullptr};
			static const GLenum option_enum[] = {GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX};
			GLenum rgb_mode, alpha_mode;
			if(lua_isnil(L, -2))
                                glGetIntegerv(GL_BLEND_EQUATION_RGB, reinterpret_cast<GLint*>(&rgb_mode));
			else
				rgb_mode = option_enum[luaL_checkoption(L, -2, nullptr, option_str)];
			if(lua_isnil(L, -1))
                                glGetIntegerv(GL_BLEND_EQUATION_ALPHA, reinterpret_cast<GLint*>(&alpha_mode));
			else
				alpha_mode = option_enum[luaL_checkoption(L, -1, nullptr, option_str)];
			glBlendEquationSeparate(rgb_mode, alpha_mode);
		}
		lua_pop(L, 2);
		// Set blend function
		lua_getfield(L, 1, "rgb_source_function");
		lua_getfield(L, 1, "rgb_destination_function");
		lua_getfield(L, 1, "alpha_source_function");
		lua_getfield(L, 1, "alpha_destination_function");
		if(!(lua_isnil(L, -4) && lua_isnil(L, -3) && lua_isnil(L, -2) && lua_isnil(L, -1))){
			static const char* option_str[] = {"0", "1", "SRCc", "1-SRCc", "DSTc", "1-DSTc", "SRCa", "1-SRCa", "DSTa", "1-DSTa", "Cc", "1-Cc", "Ca", "1-Ca", "SRCa(s)", nullptr};
			static const GLenum option_enum[] = {GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA, GL_SRC_ALPHA_SATURATE};
			GLint rgb_src_func, rgb_dst_func, alpha_src_func, alpha_dst_func;
			if(lua_isnil(L, -4))
				glGetIntegerv(GL_BLEND_SRC_RGB, reinterpret_cast<GLint*>(&rgb_src_func));
			else
				rgb_src_func = option_enum[luaL_checkoption(L, -4, nullptr, option_str)];
			if(lua_isnil(L, -3))
				glGetIntegerv(GL_BLEND_DST_RGB, reinterpret_cast<GLint*>(&rgb_dst_func));
			else
				rgb_dst_func = option_enum[luaL_checkoption(L, -3, nullptr, option_str)];
			if(lua_isnil(L, -2))
				glGetIntegerv(GL_BLEND_SRC_ALPHA, reinterpret_cast<GLint*>(&alpha_src_func));
			else
				alpha_src_func = option_enum[luaL_checkoption(L, -2, nullptr, option_str)];
			if(lua_isnil(L, -1))
				glGetIntegerv(GL_BLEND_DST_ALPHA, reinterpret_cast<GLint*>(&alpha_dst_func));
			else
				alpha_dst_func = option_enum[luaL_checkoption(L, -1, nullptr, option_str)];
			glBlendFuncSeparate(rgb_src_func, rgb_dst_func, alpha_src_func, alpha_dst_func);
			if(glHasError())	// GL_SRC_ALPHA_SATURATE just allowed for source!
				return luaL_error(L, "Invalid blend function!");
		}
		lua_pop(L, 4);
		// Enable blending
		glEnable(GL_BLEND);
	}else
		glDisable(GL_BLEND);
	return 0;
}

static int tgl_info(lua_State* L){
	TGL_CHECK_CONTEXT
	static const char* option_str[] = {"version", "extensions", nullptr};
	static const GLenum option_enum[] = {GL_VERSION, GL_EXTENSIONS};
	switch(option_enum[luaL_checkoption(L, 1, nullptr, option_str)]){
		case GL_VERSION:
                        lua_createtable(L, 0, 4);
			lua_pushstring(L, reinterpret_cast<const char*>(glGetString(GL_VENDOR))); lua_setfield(L, -2, "vendor");
			lua_pushstring(L, reinterpret_cast<const char*>(glGetString(GL_RENDERER))); lua_setfield(L, -2, "renderer");
			lua_pushstring(L, reinterpret_cast<const char*>(glGetString(GL_VERSION))); lua_setfield(L, -2, "version");
			lua_pushstring(L, reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION))); lua_setfield(L, -2, "shading_language_version");
			break;
		case GL_EXTENSIONS:{
				GLint extensions_n;
				glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_n);
				lua_createtable(L, extensions_n, 0);
					for(GLint i = 0; i < extensions_n; ++i){
						assert(glGetStringi(GL_EXTENSIONS, i));
						lua_pushstring(L, reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i))); lua_rawseti(L, -2, i);
					}
			}
			break;
	}
	return 1;
}

int luaopen_tgl(lua_State* L){
	// Initialize GLFW with general properties
	std::unique_lock<std::mutex> context_lock(context_mutex);
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
	GLFWwindow** udata = static_cast<GLFWwindow**>(lua_newuserdata(L, sizeof(GLFWwindow*)));
	*udata = window;
	// Fetch/create Lua tgl context metatable
	if(luaL_newmetatable(L, LUA_TGL_CONTEXT)){
		lua_pushcfunction(L, tgl_context_free); lua_setfield(L, -2, "__gc");
		lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, tgl_context_activate); lua_setfield(L, -2, "activate");
		lua_pushcfunction(L, tgl_shader_create); lua_setfield(L, -2, "createshader");
		lua_pushcfunction(L, tgl_program_create); lua_setfield(L, -2, "createprogram");
		lua_pushcfunction(L, tgl_vao_create); lua_setfield(L, -2, "createvao");
		lua_pushcfunction(L, tgl_texture_create); lua_setfield(L, -2, "createtexture");
		lua_pushcfunction(L, tgl_fbo_create); lua_setfield(L, -2, "createfbo");
		lua_pushcfunction(L, tgl_clear); lua_setfield(L, -2, "clear");
		lua_pushcfunction(L, tgl_viewport); lua_setfield(L, -2, "viewport");
		lua_pushcfunction(L, tgl_size); lua_setfield(L, -2, "size");
		lua_pushcfunction(L, tgl_mode); lua_setfield(L, -2, "mode");
		lua_pushcfunction(L, tgl_scissor); lua_setfield(L, -2, "scissor");
		lua_pushcfunction(L, tgl_logic); lua_setfield(L, -2, "logic");
		lua_pushcfunction(L, tgl_mask); lua_setfield(L, -2, "mask");
		lua_pushcfunction(L, tgl_depth); lua_setfield(L, -2, "depth");
		lua_pushcfunction(L, tgl_stencil); lua_setfield(L, -2, "stencil");
		lua_pushcfunction(L, tgl_blend); lua_setfield(L, -2, "blend");
		lua_pushcfunction(L, tgl_info); lua_setfield(L, -2, "info");
	}
	// Bind metatable to userdata
	lua_setmetatable(L, -2);
	// Increment counter for finally created context
	++context_count;
	// Return the userdata to Lua
	return 1;
}
