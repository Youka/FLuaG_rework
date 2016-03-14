-- Create & activate context
local ctx = require("tgl")
ctx:activate()

-- Create (+ bind) FBO as drawing target
local fbo = ctx.createfbo(_VIDEO.width, _VIDEO.height, 8)

-- Create texture for FBO blitting
local tex = ctx.createtexture(_VIDEO.width, _VIDEO.height, "rgba")

-- Create shaders program
local program = ctx.createprogram(
	ctx.createshader("vertex", [[
#version 150

in vec2 pos;
in vec3 vcol;
out vec3 fcol;

void main(void){
	fcol = vcol;
	gl_Position = vec4(pos, 0.0, 1.0);
}
	]]),
	ctx.createshader("fragment", [[
#version 150

in vec3 fcol;
out vec4 outcol;

void main(void){
	outcol = vec4(fcol, 1.0);
}
	]])
)

-- Define VAO with drawing informations
local vao = ctx.createvao(
	{
		{location = 0, size = 2},
		{location = 1, size = 3}
	},
	{
		-0.5,0, 1,0,0,
		0,0.5, 0,1,0,
		0.5,0, 0,0,1,
		0,-0.5, 1,1,0
	}
)

-- Configure drawing
fbo:bind()
ctx.viewport(0, 0, fbo:info())
program:use()

-- Draw
ctx.clear("color", 0.25, 0.25, 0.25, 1)
vao:draw("triangle fan", 0, 4)
local data = select(4, fbo:blit(tex):data(_VIDEO.has_alpha and "bgra" or "bgr"))

-- Frame processing
function GetFrame(frame)
	frame(data)
end
