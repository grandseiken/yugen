-- Enums for rendering layers.
-- Must be kept in sync with layers in game_stage.h.

local DRAW_PARALLAX0 = 0
local DRAW_PARALLAX1 = 1
local DRAW_UNDERLAY0 = 2
local DRAW_UNDERLAY1 = 3
local DRAW_WORLD = 4
local DRAW_OVERLAY0 = 5
local DRAW_SPECULAR0 = 6
local DRAW_FULLBRIGHT0 = 7
local DRAW_OVERLAY1 = 8
local DRAW_SPECULAR1 = 9
local DRAW_FULLBRIGHT1 = 10

local function render_sprite_world(
    sprite, frame_size, frame, depth)
  render_sprite(self, DRAW_WORLD, sprite, frame_size, frame,
                depth, get_rotation(self),
                1, 1, 1, 1)
end
