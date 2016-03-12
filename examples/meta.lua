local userdata = ...
local meta = string.format(
[[Userdata: %s

Video width: %d
Video height: %d
Video has alpha: %s
Video FPS: %f
Video frames: %d

Package path: %s]],
	userdata,
	_VIDEO.width,
	_VIDEO.height,
	_VIDEO.has_alpha,
	_VIDEO.fps,
	_VIDEO.frames,
	package.path
)
print(meta)

function GetFrame()
end
