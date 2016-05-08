/*
Project: FLuaG
File: glfw.hpp

Copyright (c) 2015-2016, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

// Initializes GLFW thread-safe with counter
int glfwInit_s() noexcept;
// Deinitializes GLFW thread-safe with counter
int glfwTerminate_s() noexcept;

namespace GLFW{
	// OpenGL dummy context class (minimal framebuffer for GPU access)
	class DummyContext{
		private:
			// GLFW window handle
			void* window;
		public:
			// Ctor & dtor
			DummyContext(bool set=false);
			~DummyContext() noexcept;
			// No copy
			DummyContext(const DummyContext&) = delete;
			DummyContext& operator=(const DummyContext&) = delete;
			// Move
			DummyContext(DummyContext&&) noexcept;
			DummyContext& operator=(DummyContext&&) noexcept;
			// Set active
			void set(const bool set=true) const noexcept;
	};
}
