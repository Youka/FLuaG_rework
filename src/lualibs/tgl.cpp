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
#include <algorithm>

// Lua compatibility macros
#if LUA_VERSION_NUM <= 501
	#define lua_rawlen lua_objlen
#endif

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
	// Get argument
	const bool on = lua_isnoneornil(L, 2) ? true : (luaL_checktype(L, 2, LUA_TBOOLEAN), lua_toboolean(L, 2));
	// Set current GL context to use
	glfwMakeContextCurrent(on ? *reinterpret_cast<GLFWwindow**>(luaL_checkudata(L, 1, LUA_TGL_CONTEXT)) : nullptr);
	// Prepare GL when context is current
	if(on){
		// Initialize GLEW
		glewExperimental = GL_TRUE;
		if(glewInit() != GLEW_OK)
			return luaL_error(L, "Couldn't initialize GLEW!");
		// Clear all errors caused by GLFW & GLEW initializations
		glGetError();
	}
	return 0;
}

// Shader metatable methods
static int tgl_shader_free(lua_State* L){
	glDeleteShader(*reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_SHADER)));
	return 0;
}

static int tgl_shader_create(lua_State* L){
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
	const GLuint program = *reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_PROGRAM));
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
	const GLuint vshader = *reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_SHADER)),
		fshader = *reinterpret_cast<GLuint*>(luaL_checkudata(L, 2, LUA_TGL_SHADER));
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
static int tgl_vao_free(lua_State* L){
	const GLuint* udata = reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_VAO));
	glDeleteVertexArrays(1, &udata[1]);
	glDeleteBuffers(1, &udata[0]);
	return 0;
}

static int tgl_vao_draw(lua_State* L){
	// Get arguments
	const GLuint* udata = reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_VAO));
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
	if(glGetError() == GL_INVALID_OPERATION)
		return luaL_error(L, "Drawing operation was invalid!");
	return 0;
}

static int tgl_vao_create(lua_State* L){
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
        if(glGetError() == GL_OUT_OF_MEMORY){
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
		if(glGetError() != GL_NO_ERROR){
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, &vbo);
                        return luaL_error(L, "Invalid location!");
		}
		glVertexAttribPointer(prop.location_index, prop.vertex_size, GL_FLOAT, GL_FALSE, stride, offset);
		if(glGetError() == GL_INVALID_VALUE){
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, &vbo);
                        return luaL_error(L, "Invalid vertex size!");
		}
		offset += prop.vertex_size << 2;
	}
	// Create userdata for VAO (+VBO)
	GLuint* udata = reinterpret_cast<GLuint*>(lua_newuserdata(L, sizeof(GLuint) << 1));
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
	glDeleteTextures(1, reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_TEXTURE)));
	return 0;
}

static int tgl_texture_param(lua_State* L){
	// Get arguments
	const GLuint tex = *reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_TEXTURE));
	const std::string param = luaL_checkstring(L, 2),
		value = luaL_checkstring(L, 3);
	// Bind texture for access
	glBindTexture(GL_TEXTURE_2D, tex);
	// Set texture parameters
	if(param == "filter"){
		if(value == "nearest"){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}else if(value == "linear"){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}else
			return luaL_error(L, "Invalid filter value!");
	}else if(param == "wrap"){
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
	}else
		return luaL_error(L, "Invalid parameter!");
	return 0;
}

static int tgl_texture_data(lua_State* L){
	// Get arguments
	const GLuint tex = *reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_TEXTURE));
	static const char* option_str[] = {"rgb", "bgr", "rgba", "bgra", "none", nullptr};
	static const GLenum option_enum[] = {GL_RGB, GL_BGR, GL_RGBA, GL_BGRA, 0x0};
	const GLenum request_format = option_enum[luaL_checkoption(L, 2, "none", option_str)];
	// Bind texture for access
	glBindTexture(GL_TEXTURE_2D, tex);
	// Get texture header
	GLint width, height, format;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
	// Get texture data
	std::vector<unsigned char> data;
	if(request_format){
		data.resize(width * height * (request_format == GL_RGB || request_format == GL_BGR ? 3 : 4));
		glGetTexImage(GL_TEXTURE_2D, 0, request_format, GL_UNSIGNED_BYTE, data.data());
	}
	// Send informations to Lua
	lua_pushnumber(L, width);
	lua_pushnumber(L, height);
	switch(format){
		case GL_RGB: lua_pushstring(L, "rgb"); break;
		case GL_RGBA: lua_pushstring(L, "rgba"); break;
		default: lua_pushnumber(L, format);	// Should never happen
	}
	if(data.empty())
		return 3;
	lua_pushlstring(L, reinterpret_cast<char*>(data.data()), data.size());
	return 4;
}

static int tgl_texture_bind(lua_State* L){
	// Set active texture
	glActiveTexture(GL_TEXTURE0 + luaL_optinteger(L, 2, 0));
	if(glGetError() == GL_INVALID_ENUM)
		return luaL_error(L, "Invalid unit!");
	// Bind texture
	glBindTexture(GL_TEXTURE_2D, *reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_TEXTURE)));
	// Reset active texture
	glActiveTexture(GL_TEXTURE0);
	return 0;
}

static int tgl_texture_create(lua_State* L){
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
	if(glGetError() == GL_INVALID_VALUE){
		glDeleteTextures(1, &tex);
		return luaL_error(L, "Invalid texture value!");
	}
	glGenerateMipmap(GL_TEXTURE_2D);
	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// Create userdata for texture
	*reinterpret_cast<GLuint*>(lua_newuserdata(L, sizeof(GLuint))) = tex;
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
	const GLuint* udata = reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_FBO));
	glDeleteFramebuffers(1, &udata[2]);
	glDeleteRenderbuffers(2, &udata[0]);
	return 0;
}

static int tgl_fbo_bind(lua_State *L){
	glBindFramebuffer(GL_FRAMEBUFFER, *reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_FBO)));
	return 0;
}

static int tgl_fbo_info(lua_State *L){
	// Temporary bind RBO and get data
	glBindRenderbuffer(GL_RENDERBUFFER, *reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_FBO)));
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

static int tgl_fbo_to_texture(lua_State *L){
	// Get argument
	const GLuint* udata = reinterpret_cast<GLuint*>(luaL_checkudata(L, 1, LUA_TGL_FBO));
	// Get source FBO dimensions
	glBindRenderbuffer(GL_RENDERBUFFER, udata[0]);
	GLint width, height;
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
	// Generate & prepare output texture
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	if(glGetError() == GL_INVALID_VALUE){
		glDeleteTextures(1, &tex);
		return luaL_error(L, "Couldn't generate texture!");
	}
	// Generate & bind destination FBO
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	// Bind source FBO
	glBindFramebuffer(GL_READ_FRAMEBUFFER, udata[2]);
	// Blit source to destination FBO (includes downsampling)
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	if(glGetError() == GL_INVALID_OPERATION){
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &fbo);
		glDeleteTextures(1, &tex);
		return luaL_error(L, "Couldn't copy framebuffer to texture data!");
	}
	// Reset FBOs to default
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Delete no longer needed FBO
	glDeleteFramebuffers(1, &fbo);
	// Configure texture
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// Create userdata for texture
	*reinterpret_cast<GLuint*>(lua_newuserdata(L, sizeof(GLuint))) = tex;
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

static int tgl_fbo_create(lua_State* L){
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
	const GLenum err = glGetError();
	if(err != GL_NO_ERROR){
		glDeleteRenderbuffers(2, rbo);
		return luaL_error(L, err == GL_INVALID_VALUE ? "Invalid dimensions!" : "Couldn't allocate memory for FBO buffers!");
	}
	// Generate FBO
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	// Bind RBOS to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo[0]);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);
	// Reset FBO to default
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Create userdata for FBO
	GLuint* udata = reinterpret_cast<GLuint*>(lua_newuserdata(L, sizeof(GLuint) * 3));
	udata[0] = rbo[0];
	udata[1] = rbo[1];
	udata[2] = fbo;
	// Fetch/create Lua tgl fbo metatable
	if(luaL_newmetatable(L, LUA_TGL_FBO)){
		lua_pushcfunction(L, tgl_fbo_free); lua_setfield(L, -2, "__gc");
		lua_pushvalue(L, -1); lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, tgl_fbo_bind); lua_setfield(L, -2, "bind");
		lua_pushcfunction(L, tgl_fbo_info); lua_setfield(L, -2, "info");
		lua_pushcfunction(L, tgl_fbo_to_texture); lua_setfield(L, -2, "totexture");
	}
	// Bind metatable to userdata
	lua_setmetatable(L, -2);
	// Return the userdata to Lua
	return 1;
}

static int tgl_clear(lua_State* L){
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
			glClearDepth(luaL_optnumber(L, 2, 0));
			break;
		case GL_STENCIL_BUFFER_BIT:
			glClearStencil(luaL_optinteger(L, 2, 0));
			break;
	}
	glClear(buffer);
	return 0;
}

static int tgl_viewport(lua_State* L){
	glViewport(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4));
        if(glGetError() == GL_INVALID_VALUE)
		return luaL_error(L, "Invalid rectangle!");
	return 0;
}

// TODO: depth, stencil, blend, raster, viewport

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
		lua_pushcfunction(L, tgl_texture_create); lua_setfield(L, -2, "createtexture");
		lua_pushcfunction(L, tgl_fbo_create); lua_setfield(L, -2, "createfbo");
		lua_pushcfunction(L, tgl_clear); lua_setfield(L, -2, "clear");
		lua_pushcfunction(L, tgl_viewport); lua_setfield(L, -2, "viewport");
	}
	// Bind metatable to userdata
	lua_setmetatable(L, -2);
	// Increment counter for finally created context
	++context_count;
	// Return the userdata to Lua
	return 1;
}
