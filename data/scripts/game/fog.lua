-- Test fog script.
#include "../render.lua"

local frame = 0
function update()
  frame = 1 + frame
end

function draw()
  render_fog_table({
      layer = DRAW_OVERLAY0,
      layering_value = 0.7,
      origin = get_origin(self),
      region = get_region(self),
      tex_offset = vec(frame / 4, -frame / 8),
      frame = frame / 8,
      colour = colour(.5, .5, .5, .25),
      fog_min = 0.4,
      fog_max = 0.7})
end
