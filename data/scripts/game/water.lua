-- Test fog script.
#include "../render.lua"
#include "../script_set.lua"

function yedit_colour()
  return .3, .6, .8
end

local function submerge(amount)
  return function(script)
    script:send_message("on_submerge", amount)
  end
end

local function emerge(script)
  script:send_message("on_emerge")
end

local scripts_submerging = {}
local scripts_submerged = {}
local frame = 0
local submerge_dist = 128

function update()
  local origin = self:get_origin()
  local region = self:get_region()
  local v = vec(submerge_dist, submerge_dist)

  local new_submerging = get_scripts_in_region(origin, region + v / 2)
  local new_submerged = {}

  local min = origin - region / 2 - v / 4
  local max = origin + region / 2 + v / 4

  for i, script in ipairs(new_submerging) do
    local o = script:get_origin()
    if o:in_region(origin, region - v / 2) then
      new_submerged[1 + #new_submerged] = script
    else
      dist = math.min(math.abs(o:x() - min:x()),
                      math.abs(o:x() - max:x()),
                      math.abs(o:y() - min:y()),
                      math.abs(o:y() - max:y()))
      submerge(2 * dist / submerge_dist)(script)
    end
  end

  local nothing = function() end
  scripts_submerged = update_script_set(
      scripts_submerged, new_submerged, submerge(1), nothing)
  scripts_submerging = update_script_set(
      scripts_submerging, new_submerging, nothing, emerge)
  frame = 1 + frame
end

-- Water needs to be drawn on a FULLBRIGHT layer for the lighting inside to be
-- properly distorted.
function draw()
  local o = self:get_origin()
  local r = self:get_region()

  -- The main water layer.
  table = {
      layer = DRAW_FULLBRIGHT1,
      layering_value = .3,
      origin = o,
      region = r,
      tex_offset = vec(frame / 16, 0),
      frame = frame / 16,
      colour = colour(.3, .6, .8, .4),
      light_passthrough = .8,
      reflect_mix = .5,
      normal_scaling = .1,
      normal_scaling_reflect = .2,
      normal_scaling_refract = 2.5,
      reflect_fade_start = 128,
      reflect_fade_end = 384,
      flip_x = false,
      flip_y = true,
      flip_axes = o - r / 2,
      wave_height = 4,
      wave_scale = .5}

  render_reflect_table(table)

  -- Mess with the normals a bit underwater to simulate light passing through
  -- liquid.
  table.layer = DRAW_WORLD
  table.layering_value = .5
  table.normal_scaling = .2
  table.normal_only = true

  render_reflect_table(table)

  -- Specular overlay.
  table.layer = DRAW_SPECULAR2
  table.layering_value = .2
  table.colour = colour(1, 1, 1, .2)
  table.light_passthrough = 0
  table.reflect_mix = 0
  table.normal_scaling = .75
  table.normal_scaling_reflect = 0
  table.normal_scaling_refract = 0
  table.normal_only = false

  render_reflect_table(table)
end
