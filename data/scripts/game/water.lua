-- Test fog script.
#include "../render.lua"

local frame = 0
function update()
  frame = 1 + frame
end

function draw()
  local o = get_origin(self)
  local r = get_region(self)
  render_reflect(DRAW_OVERLAY1, o, r,
                 vec(frame / 16, 0), frame / 16, .3, .6, .8, .4,
                 false, true, o - r / vec(2, 2))
end
