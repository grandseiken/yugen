-- Test fog script.
#include "../render.lua"
#include "../script_set.lua"

local scripts_in_water = {}
local function add_function(script)
  print "entered water"
end

local function remove_function(script)
  print "left water"
end

local frame = 0
function update()
  new_in_water = get_scripts_in_region(get_origin(self),
                                       get_region(self))
  scripts_in_water = update_script_set(
      scripts_in_water, new_in_water, add_function, remove_function)
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
