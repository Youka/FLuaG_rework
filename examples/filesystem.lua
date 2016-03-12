local fs = require("filesystem")

for _, file in ipairs(fs.dir(".")) do
	print(string.format("Path: %s\nSize: %d\nLast modification: %s\nPermissions: %s\nType: %s\n",
			fs.canonical(file), fs.size(file), fs.lastmod(file), table.concat({fs.perm(file)}, " "), fs.type(file)))
end

function GetFrame()
end
