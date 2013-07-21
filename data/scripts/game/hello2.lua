-- A thing that is made by the test thing.
#include "../render.lua"

local sprite = get_sprite("/tiles/temple.png")
local script = get_script("/scripts/hello.lua")
local counter = 0
math.randomseed(os.time() + self:get_origin():x() + self:get_origin():y() * 17)
local r = math.floor(100 * math.random())
function update()
  local o = vec(1, -1) + self:get_origin()
  self:set_origin(o)
  counter = counter + 1
  if counter == r then
  -- create(script, o)
  end
  if counter == 80 then
    self:destroy()
  end
end
function draw()
  render_sprite_self(sprite, vec(32, 32), vec(1, 0), 0.0)
end
