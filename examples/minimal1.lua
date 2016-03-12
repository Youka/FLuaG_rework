-- Process frame
function GetFrame(frame)
	-- Detect video is BGR or BGRA
	if _VIDEO.has_alpha then
		-- Set frame to opaque purple
		frame(string.rep("\255\0\127\255", #frame/4))
	else
		-- Set frame to purple
		frame(string.rep("\255\0\127", #frame/3))
	end
end
