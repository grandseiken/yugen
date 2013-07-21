-- A crate. It falls.
#include "../collide.lua"
#include "../render.lua"

local sprite = get_sprite("/tiles/ruin.png")
local body = self:create_body(vec(0, 0), vec(32, 32))
body:set_collide_mask(COLLIDE_WORLD)
local test = self:create_light(50, 50)

function on_submerge(amount)
  test:set_layer_value(.2 + .05 * amount)
end
function on_emerge()
  test:set_layer_value(0)
end

function update()
  self:collider_move(vec(0, 3))
end
function draw()
  render_sprite_self(sprite, vec(32, 32), vec(4, 3), 0.3)
end
