require("utf8x")

local text = "こんにちは"
print(string.format("Text: %s\nLength: %s", text, utf8.len(text)))
for char, byte_pos in utf8.chars(text) do
	print(string.format("%s: %s", byte_pos, char))
end

function GetFrame(frame)
end
