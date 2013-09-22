-- A bush. It walks about, for some reason.
#include "../collide.lua"
#include "../render.lua"

function yedit_colour()
  return .5, .5, .5
end

local sprite = get_sprite("/tiles/easy.png")
local body = self:create_body(vec(0, 0), vec(2, 2))
body:set_collide_type(0)
body:set_collide_mask(COLLIDE_WORLD)
local hooked = false

-- Check body for checks.
self:create_body(vec(0, 0), vec(4, 4))

function update()
  if hooked then
    return
  end
  if self:source_check(0, COLLIDE_WORLD) then
    hooked = true
  end
  self:collider_move(vec(0, -4))
end
function draw()
  render_sprite_self(sprite, vec(32, 32), vec(2, 15), 0.3)
end
