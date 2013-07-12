-- A crate. It falls.
#include "../collide.lua"
#include "../render.lua"

local sprite = get_sprite("/tiles/ruin.png")
local body = create_body(self, vec(0, 0), vec(32, 32))
set_collide_mask(body, COLLIDE_WORLD)
local test = create_light(self, 50, 50)

function on_enter_water()
  set_light_layer_value(test, .25)
end
function on_leave_water()
  set_light_layer_value(test, 0)
end

function update()
  collider_move(self, vec(0, 3))
end
function draw()
  render_sprite_world(sprite, vec(32, 32), vec(4, 3), 0.3)
end
