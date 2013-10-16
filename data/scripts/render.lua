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
local DRAW_OVERLAY2 = 11
local DRAW_SPECULAR2 = 12
local DRAW_FULLBRIGHT2 = 13

-- Convenience functions to call the rendering API functions with keyword
-- arguments stored in a table.

local function colour(r, g, b, a)
  return {r = r, g = g, b = b, a = a}
end

local function render_sprite_table(table)
  render_sprite(
      table.layer, table.depth,
      table.origin, table.rotation,
      table.sprite, table.frame_size, table.frame,
      table.colour.r, table.colour.g, table.colour.b, table.colour.a)
end

local function render_fog_table(table)
  render_fog(
      table.layer, table.layering_value,
      table.origin, table.region,
      table.tex_offset, table.frame,
      table.colour.r, table.colour.g, table.colour.b, table.colour.a,
      table.fog_min, table.fog_max, table.normal_only)
end

local function render_reflect_table(table)
  render_reflect(
      table.layer, table.layering_value,
      table.origin, table.region,
      table.tex_offset, table.frame,
      table.colour.r, table.colour.g, table.colour.b, table.colour.a,
      table.reflect_mix, table.light_passthrough, table.normal_scaling,
      table.normal_scaling_reflect, table.normal_scaling_refract,
      table.reflect_fade_start, table.reflect_fade_end,
      table.flip_x, table.flip_y, table.flip_axes,
      table.wave_height, table.wave_scale, table.normal_only)
end

-- Convenience functions for common operations.

local function render_sprite_self(
    sprite, frame_size, frame, depth)
  render_sprite_table({
      layer = DRAW_WORLD,
      depth = depth,
      origin = self:get_origin(),
      rotation = self:get_rotation(),
      sprite = sprite,
      frame_size = frame_size,
      frame = frame,
      colour = colour(1, 1, 1, 1)})
end
