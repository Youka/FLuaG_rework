require("mathx")

print(math.round(-1.5))
print(math.trunc(-1.5))

print(math.fac(5))

print(math.gauss(-0.5, 4.5))

for i=0, 1, 0.1 do
	print(math.bezier(i, 0, 3.3, 6.6, 10))
end

print(math.complex(0,1) + math.complex(2,1))

function GetFrame(frame)
end
