/*
Project: FLuaG
File: glfw.cpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "glfw.hpp"
#include <GLFW/glfw3.h>
#include <mutex>
#include <stdexcept>

static unsigned glfw_init_count = 0;
static std::mutex glfw_init_mutex;

int glfwInit_s() noexcept{
	std::unique_lock<std::mutex> lock(glfw_init_mutex);
	if(glfw_init_count == 0 && !glfwInit())
		return -1;
	return ++glfw_init_count;
}

int glfwTerminate_s() noexcept{
	std::unique_lock<std::mutex> lock(glfw_init_mutex);
	switch(glfw_init_count){
		case 0: return 0;
		case 1: glfwTerminate(); break;
	}
	return --glfw_init_count;
}

namespace GLFW{
	DummyContext::DummyContext(bool set){
		// Initialize GLFW dependency
		if(glfwInit_s() < 0)
			throw std::domain_error("Couldn't initialize GLFW!");
		// Hints for very simple window
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
		glfwWindowHint(GLFW_DECORATED, GL_FALSE);
		glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		// Create window (+context)
		this->window = glfwCreateWindow(1, 1, "Dummy", nullptr, nullptr);
		if(!this->window){
			glfwTerminate_s();
			throw std::domain_error("Couldn't create GL window!");
		}
		// Make context already current?
		if(set)
			this->set(true);
	}
	DummyContext::~DummyContext() noexcept{
		if(this->window){
			glfwDestroyWindow(static_cast<GLFWwindow*>(this->window));
			glfwTerminate_s();
		}
	}

	DummyContext::DummyContext(DummyContext&& other) noexcept{
		*this = std::move(other);
	}
	DummyContext& DummyContext::operator=(DummyContext&& other) noexcept{
		this->window = other.window;
		other.window = nullptr;
		return *this;
	}

	void DummyContext::set(const bool set) const noexcept{
		if(this->window)
			glfwMakeContextCurrent(set ? static_cast<GLFWwindow*>(this->window) : nullptr);
	}
}
