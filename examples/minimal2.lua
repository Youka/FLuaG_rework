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
