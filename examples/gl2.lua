-- Create & activate context
local ctx = require("tgl")
ctx:activate()

-- Create & set framebuffer
local fbo = ctx.createfbo(_VIDEO.width, _VIDEO.height, 8)
fbo:bind()
ctx.viewport(0, 0, fbo:info())

-- Create projection & modelview matrix
local gm = require("geometry")
local proj = gm.matrix():translate(-1, 1, 0):scale(2/_VIDEO.width, -2/_VIDEO.height,1/10000)
local model = gm.matrix()

-- Create & use shaders program
local program = ctx.createprogram(
	ctx.createshader("vertex", [[
#version 150

in vec3 pos;
in vec2 tex_coord_v;
out vec2 tex_coord;
uniform mat4 proj, model;

void main(void){
	tex_coord = tex_coord_v;
	gl_Position = vec4(pos, 1.0) * model * proj;
}
	]]),
	ctx.createshader("fragment", [[
#version 150

in vec2 tex_coord;
out vec4 outcol;
uniform sampler2D tex;

void main(void){
	outcol = texture(tex, tex_coord);
}
	]])
)
program:use()
program:uniform("proj", "mat", proj:data())
program:uniform("tex", "i", 0)

-- Define VAO for cube vertices + texture coordinates
local size = 200
local vao = ctx.createvao(
	{
		{location = 0, size = 3},
		{location = 1, size = 2}
	},
	{
		-- Front
		-size,-size,-size, 0,1,
		size,-size,-size, 1,1,
		size,size,-size, 1,0,
		-size,size,-size, 0,0,
		-- Back
		size,-size,size, 0,1,
		-size,-size,size, 1,1,
		-size,size,size, 1,0,
		size,size,size, 0,0,
		-- Top
		-size,-size,size, 0,1,
		size,-size,size, 1,1,
		size,-size,-size, 1,0,
		-size,-size,-size, 0,0,
		-- Bottom
		-size,size,-size, 0,1,
		size,size,-size, 1,1,
		size,size,size, 1,0,
		-size,size,size, 0,0,
		-- Left
		-size,size,size, 0,1,
		-size,size,-size, 1,1,
		-size,-size,-size, 1,0,
		-size,-size,size, 0,0,
		-- Right
		size,size,-size, 0,1,
		size,size,size, 1,1,
		size,-size,size, 1,0,
		size,-size,-size, 0,0
	}
)

-- Enable depth test
ctx.depth("less equal")

-- Frame processing
function GetFrame(frame, ms)
	-- Clear framebuffer
	ctx.clear("color")
	ctx.clear("depth")
	-- Bind frame as texture
	local tex = ctx.createtexture(_VIDEO.width, _VIDEO.height, _VIDEO.has_alpha and "bgra" or "bgr", frame())
	tex:bind()
	-- Set modelview transformation
	program:uniform("model", "mat", model:identity():translate(_VIDEO.width/2,_VIDEO.height/2,0):rotate("x", ms/2000):rotate("y", ms/1500):rotate("z", ms/1000):data())
	-- Draw animated textured cube
	vao:draw("triangle fan", 0, 4)
	vao:draw("triangle fan", 4, 4)
	vao:draw("triangle fan", 8, 4)
	vao:draw("triangle fan", 12, 4)
	vao:draw("triangle fan", 16, 4)
	vao:draw("triangle fan", 20, 4)
	-- Copy framebuffer to frame
	frame(select(4, fbo:blit(tex):data(_VIDEO.has_alpha and "bgra" or "bgr")))
end
