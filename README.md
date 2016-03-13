# FLuaG
**FLuaG** (*Floating Lua Graphics*) is a *video frameserver* plugin ([Avisynth](http://avisynth.nl) + [Vapoursynth](http://www.vapoursynth.com/)) for linear video editing by scripting language [Lua](http://www.lua.org/).

### Features
- Cross-platform
- Core in one file
- Filtering of BGR(A) frame data from popular video frameservers
- C API interface for custom hosts and general purpose usage of Lua
- An extended Lua environment with module search path at plugin location and libraries for math, graphics, files, etc. for more effective work with data
- OpenGL3 access for fast 3D rendering
- Customizable build: choose your Lua interpreter, if Lua should always run in the same system thread, use specific hardware vectorization, etc.

### Build & installation
FLuaG can be build & installed completely by <a href="https://cmake.org/">CMake</a>. C++11 capable MSVC (11+) and GCC (4.8+) compilers are supported. Dependencies are system or proven cross-platform libraries. For more, see the project CMake configuration.

### Getting started
Because FLuaG requires users to write program code in Lua, learning this language would be the first step. [Tutorialspoint](http://www.tutorialspoint.com/lua/) provides a good tutorial but better would be reading [books](http://www.lua.org/docs.html#books).

Furthermore FLuaG is used as client of a host, so knowing the host is another important point. As mentioned, [Avisynth](http://avisynth.nl/index.php/Getting_started) and [Vapoursynth](http://www.vapoursynth.com/doc/gettingstarted.html) are the usual targets.

Next reading the [documentations](docs/) and playing with examples *(see below)* simultaneously is the way to learn.

### Examples
---
**test.vpy**

	# Import Vapoursynth
	import vapoursynth as vs

	# Get Vapoursynth core
	core = vs.get_core()

	# Create grey clip
	clip = core.std.BlankClip(width = 1280, height = 720, format = vs.RGB24, fpsnum = 24000, fpsden = 1001, length = 240, color = [127, 127, 127])
	# Edit clip by FLuaG
	core.std.LoadPlugin("<INSERT_PATH_HERE>/libfluag.so")
	clip = core.graphics.FLuaG(clip, "test.lua")
	# Set output clip
	core.resize.Bicubic(clip, format=vs.YUV420P8, matrix_s="709").set_output()

**test.avs**

	# Load FLuaG plugin
	LoadCPlugin("<INSERT_PATH_HERE>/libfluag.dll")

	# Create simple clip
	ColorBars().KillAudio().Trim(0,300)
	# Edit clip by FLuaG
	FLuaG("test.lua")

**test.lua** *(if compiled with [LuaJIT](http://luajit.org/luajit.html) as Lua interpreter)*

	-- Load FFI library
	local ffi = require("ffi")

	-- Process frame
	function GetFrame(frame)
		-- Fetch frame data
		local data, data_size = frame(), #frame

		-- Edit frame data
		local cdata = ffi.cast("uint8_t*", ffi.cast("const char*", data))
		for i=0, data_size-1, _VIDEO.has_alpha and 4 or 3 do
			-- Set pixel to red'ish color
			cdata[i+2] = 255
		end

		-- Set frame data
		frame(data)
	end
---
For more, take a look at the [examples directory](examples/).

### Contact
Just whisper to **Youka** on <a href="http://en.wikipedia.org/wiki/IRC">IRC</a> servers [freenode](https://www.freenode.net/) or [rizon](http://rizon.net/) or open an issue on [Github](https://github.com/Youka/FLuaG).

### Contributing
FLuaG stays in full control of Youka, so becoming a lead developer is out of question. But giving suggestions and ideas or sending patches/pull requests is appreciated.

### License
This software is under zlib license. For details, read [LICENSE.txt](LICENSE.txt).

### Changelog

