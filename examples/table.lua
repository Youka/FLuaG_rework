require("tablex")

print("Original table")
local t = table.alloc(5, 2)
t[1] = 5
t[2] = 4
t[3] = 3
t[4] = 2
t[5] = 1
t.a = "B"
t.b = "A"
print(table.tostring(t))

print("Reversed")
table.reverse(t)
print(table.tostring(t))

print("3 elements removed at index 2")
table.removen(t, 2, 3)
print(table.tostring(t))

print("Inserted 3 new values at index 2")
table.insertn(t, 2, -2, 3, 4)
print(table.tostring(t))

print("Accumulated elements")
print(table.accumulate(t, 0, function(init, x) return init + x end))

print("Element position of -2")
print(table.find(t, -2))

function GetFrame(frame)
end
