-- Enums for rendering layers.
-- Must be kept in sync with layers in game_stage.h.

local DRAW_UNDERLAY0 = 0
local DRAW_UNDERLAY1 = 1
local DRAW_WORLD = 2
local DRAW_OVERLAY0 = 3
local DRAW_OVERLAY1 = 4

local function render_sprite_world(
    sprite, frame_size, frame, depth)
  render_sprite(self, DRAW_WORLD, sprite, frame_size, frame,
                depth, get_rotation(self),
                1, 1, 1, 1)
end
