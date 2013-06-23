-- Test fog script.
#include "../render.lua"

local frame = 0
function update()
  frame = 1 + frame
end

function draw()
  render_fog(DRAW_OVERLAY0, get_origin(self), get_region(self),
             vec(frame / 4, -frame / 8), frame / 8, .5, .5, .5, .25, 0.4, 0.7)
end
