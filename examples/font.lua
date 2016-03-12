local fonts = require("font")
require("tablex")

for _, font in ipairs(fonts.list()) do
	print(table.tostring(font))
end

local font, text = fonts.create("Arial"), "Hi"
print(string.format("Family: %s\nSize: %d\nBold: %s\nItalic: %s\nUnderline: %s\nStrikeout: %s\nSpacing: %f\nRight-to-left: %s", font:data()))
print(string.format("Height: %f\nAscent: %f\nDescent: %f\nInternal leading: %f\nExternal leading: %f", font:metrics()))
print("Text: " .. text)
print("Width: " .. font:textwidth(text))
print(table.tostring(font:textpath(text)))

function GetFrame()
end
