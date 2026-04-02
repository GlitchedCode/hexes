-- scripts/example.lua
-- Hot-reload demo module. Edit and save this file while the app is running
-- to trigger a reload — the new greet() output will appear immediately.

local M = {}

M.greet = function(name)
    return string.format("hello from Lua, %s! (Lua %s)", name, _VERSION)
end

-- This line runs every time the module is loaded or reloaded.
print("[example.lua] module loaded")

return M
