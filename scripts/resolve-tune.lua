local replacements = {}

local function trim(s)
  return s:gsub("^%s+", ""):gsub("%s+$", "")
end

for line in io.lines() do
  line = trim(line)
  if line ~= "" then
    local i = line:find(",")
    local name, value = line:sub(1, i-1), line:sub(i+1)
    replacements[trim(name)] = trim(value)
  end
end

local function read_file(path)
  local file = io.open(path, "r")
  local content = file:read("*a")
  file:close()
  return content
end

local function write_file(path, content)
  local file = io.open(path, "w")
  file:write(content)
  file:close()
  return true
end

local function do_file(path)
  local contents = read_file(path)
  for name, value in pairs(replacements) do
    contents = contents:gsub("(tune::" .. name .. ")([^%d])", value .. "_z%2")
  end
  write_file(path, contents)
  print("Done " .. path)
end

do_file("src/rose/search.cpp")
do_file("src/rose/move_picker.cpp")
