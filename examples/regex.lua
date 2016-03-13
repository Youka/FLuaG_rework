local regex = require("regex")
require("tablex")

local pattern = regex("([a-z]+)([0-9]|!)", {"icase", "optimize"})
print(table.tostring(pattern:match("abc1def?GHI!")))
print(pattern:replace("Hello1 world2!", "\\2\\1"))

function GetFrame(frame)
end
