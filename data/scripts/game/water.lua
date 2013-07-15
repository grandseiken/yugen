-- Test fog script.
#include "../render.lua"
#include "../script_set.lua"

local function submerge(amount)
  return function(script)
    send_message(script, "on_submerge", {amount})
  end
end

local function emerge(script)
  send_message(script, "on_emerge", {})
end

local scripts_submerging = {}
local scripts_submerged = {}
local frame = 0
local submerge_dist = 128

function update()
  local origin = get_origin(self)
  local region = get_region(self)
  local new_submerging = get_scripts_in_region(origin, region)
  local new_submerged = {}

  local v = vec(submerge_dist, submerge_dist)
  local left = (origin - region / vec(2, 2)):x()
  local right = (origin + region / vec(2, 2)):x()
  local top = (origin - region / vec(2, 2)):y()
  local bottom = (origin + region / vec(2, 2)):y()

  for i, script in ipairs(new_submerging) do
    local o = get_origin(script)
    if o:in_region(origin, region - v) then
      new_submerged[1 + #new_submerged] = script
    else
      dist = math.min(math.abs(o:x() - left),
                      math.abs(o:x() - right),
                      math.abs(o:y() - top),
                      math.abs(o:y() - bottom))
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

function draw()
  local o = get_origin(self)
  local r = get_region(self)
  render_reflect(DRAW_SPECULAR1, 0.2, o, r,
                 vec(frame / 16, 0), frame / 16, 1, 1, 1, .2,
                 0, .75, 0, 0, 128, 384,
                 false, true, o - r / vec(2, 2), 4, .5)
  render_reflect(DRAW_OVERLAY1, 0.3, o, r,
                 vec(frame / 16, 0), frame / 16, .3, .6, .8, .4,
                 .5, .1, .2, 2.5, 128, 384,
                 false, true, o - r / vec(2, 2), 4, .5)
end
