-- Test fog script.
#include "../render.lua"

local frame = 0
function update()
  frame = 1 + frame
end

function draw()
  render_fog(get_origin(self), get_region(self), DRAW_OVERLAY0,
             frame, .5, .5, .5, .25)
end
