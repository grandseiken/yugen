-- player.lua --
#include "keys.lua"

local sprite = get_sprite("/tiles/easy.png")
local frame_size = vec(32, 32)
local frame = vec(3, 16)
local body = create_body(self, vec(0, 8), vec(6, 16))

local gravity = 4

local jump_period = 30
local jump_cap_period = 10
local jump_counter = 0

function update()
  local v = 0
  if is_key_down(KEY_LEFT) then
    v = v - 2
  end
  if is_key_down(KEY_RIGHT) then
    v = v + 2
  end
  collider_move(self, vec(0, -2))
  collider_move(self, vec(v, 0))
  collider_move(self, vec(0, 2))

  if not is_key_down(KEY_UP) and
      jump_counter > 2 * jump_cap_period and
      jump_counter < jump_cap_period + jump_period then
    jump_counter = 2 * jump_cap_period
  end

  if jump_counter > 2 * jump_cap_period then
    v = -1
  elseif jump_counter > jump_cap_period then
    v = -(jump_counter - jump_cap_period) / jump_cap_period
  elseif jump_counter > 0 then
    v = (jump_cap_period - jump_counter) / jump_cap_period
  else
    v = 1
  end

  if jump_counter > 0 then
    jump_counter = jump_counter - 1
  end
  collider_move(self, vec(0, v * gravity))
end

function key(k)
  if k == KEY_UP and jump_counter == 0 then
    jump_counter = jump_period + 2 * jump_cap_period
  end
end

function draw()
  render_sprite(self, sprite, frame_size, frame, 0.0)
end
