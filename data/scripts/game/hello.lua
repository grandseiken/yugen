-- A test thing that makes other things.
#include "../render.lua"

local sprite = get_sprite("/tiles/temple.png")
local script = get_script("/scripts/game/hello2.lua")
local counter = 0
local body = self:create_body(vec(0, 8), vec(32, 16))
function update()
  self:collider_move(vec(1, 1))
  counter = counter + 1
  if counter >= 64 then
    counter = counter - 64
    create_script(script, self:get_origin())
  end
end
function draw()
  render_sprite_self(sprite, vec(32, 32), vec(0, 0), 0.0)
end
