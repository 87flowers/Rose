local function replace_tunables(code, name_prefix, udl_postfix)
  local mapping = {}
  local counter = 0

  local function replace(num)
    local name = name_prefix .. string.format("%02d", counter)
    counter = counter + 1

    local type = ""
    if string.find(num, "[.]") then
      type = "f64"
    else
      type = "i32"
    end

    mapping[name] = { value = num, type = type }
    return "tune::" .. name
  end

  code = code:gsub("([+-]?%d+[.]?%d*)" .. udl_postfix, replace)

  return code, mapping
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

local function do_file(path, name_prefix)
  local input = read_file(path)
  local output, mapping = replace_tunables(input, name_prefix, "_z")
  write_file(path .. ".bak", input)
  write_file(path, output)

  for k, v in pairs(mapping) do
    local value = tonumber(v.value)
    local a = 0
    local b = value * 2
    print("x(" .. v.type .. ", " .. k .. ", " .. v.value .. ", " .. math.min(a, b) .. ", " .. math.max(a, b) .. ", " .. math.abs(value * 0.1)  .. ", " .. 0.002 .. ") \\")
  end
end

do_file("src/rose/search.cpp", "search")
do_file("src/rose/move_picker.cpp", "mp")
