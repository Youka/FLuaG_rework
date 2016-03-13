local gm = require("geometry")
require("tablex")

print(gm.intriangle(2.5,2.49, 0,0, 5,0, 0,5))
print(gm.lineintersect(0,0, 6,6, 5,0, 0,5))

print(table.tostring(gm.arccurve(0,0, 5,0, math.pi)))
print(table.tostring(gm.curveflatten(0,0, 0,-2, 3,-2, 3,0, 0.01)))

print(table.tostring(gm.tesselate({
	{0,0, 10,0, 10,10, 0,10},
	{2,2, 8,2, 8,8, 2,8}
})))

print(gm.matrix():rotate("y", -math.pi):translate(2,0,0):transform(0,0,0,1))

function GetFrame(frame)
end
