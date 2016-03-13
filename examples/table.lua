require("tablex")

local t = table.alloc(5, 2)
t[1] = 5
t[2] = 4
t[3] = 3
t[4] = 2
t[5] = 1
t.a = "B"
t.b = "A"

table.reverse(t)
print(table.tostring(t))

table.removen(t, 2, 3)
print(table.tostring(t))

table.insertn(t, 2, -2, 3, 4)
print(table.tostring(t))

print(table.accumulate(t, 0, function(init, x) return init + x end))

print(table.find(t, -2))

function GetFrame(frame)
end
