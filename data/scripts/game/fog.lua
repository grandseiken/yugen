-- Test fog script.
#include "../render.lua"

function yedit_colour()
  return .3, .3, .3
end

local frame = 0
function update()
  frame = 1 + frame
end

function draw()
  render_fog_table({
      layer = DRAW_OVERLAY0,
      layering_value = 0.7,
      origin = self:get_origin(),
      region = self:get_region(),
      tex_offset = vec(frame / 4, -frame / 8),
      frame = frame / 8,
      colour = colour(.5, .5, .5, .25),
      fog_min = 0.4,
      fog_max = 0.7})
end
