-- Load PNG library and math extensions
local png = require("png")
require("mathx")

-- Image size & data
local size, pixels, pixels_n = 200, {}, 0
-- Fill image data
local half_size = size / 2
for y = 0.5, size-0.5 do
	for x = 0.5, size-0.5 do
			-- Calculate & insert pixel to image data
			local distance = math.hypot(half_size - x, half_size - y)
			pixels_n = pixels_n + 1
			pixels[pixels_n] = string.char(255, 255, 255, distance > half_size and 0 or (1 - distance / half_size) * 255)
	end
end
-- Write image to file
png.writefile("light.png", {width = size, height = size, type = "rgba", data = table.concat(pixels)})

-- No frame processing
function GetFrame(frame, ms)
end