-- A bush. It walks about, for some reason.
#include "../collide.lua"
#include "../render.lua"

function yedit_colour()
  return .2, .8, .4
end

local sprite = get_sprite("/tiles/easy.png")
local body = self:create_body(vec(0, 4), vec(24, 24))
body:set_collide_type(COLLIDE_OBJECT + COLLIDE_PUSHABLE)
body:set_collide_mask(COLLIDE_WORLD + COLLIDE_OBJECT + COLLIDE_PLAYER)

local dir = true
function update()
  local d = vec(dir and 1 or -1, 0)
  if self:collider_move(d):length_squared() < 0.5 then
    dir = not dir
  end
  self:collider_move(vec(0, 3))
end
function draw()
  render_sprite_self(sprite, vec(32, 32), vec(2, 15), 0.3)
end
