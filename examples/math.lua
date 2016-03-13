require("mathx")

print("Round -1.5: " .. math.round(-1.5))
print("Trunc -1.5: " .. math.trunc(-1.5))

print("Factorial 5: " .. math.fac(5))

print("Gauss (value -0.5, sigma 4.5): " .. math.gauss(-0.5, 4.5))

print("Bezier at time 0.5 with points 0,6,10: " .. math.bezier(0.5, 0, 6, 10))

print("Complex (0,1) + (2,1): " .. tostring(math.complex(0,1) + math.complex(2,1)))

function GetFrame(frame)
end
