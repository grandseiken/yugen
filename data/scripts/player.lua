-- player.lua --
#include "keys.lua"

local sprite = get_sprite("/tiles/easy.png")
local frame_size = vec(32, 32)
local frame = vec(3, 16)
local body = create_body(self, vec(0, 8), vec(6, 16))
local jump_time = 0

function update()
  local v = vec(0, 0)
  if is_key_down(KEY_LEFT) then
    v = v + vec(-2, 0)
  end
  if is_key_down(KEY_RIGHT) then
    v = v + vec(2, 0)
  end
  if jump_time > 0 then
    collider_move(self, vec(0, -3))
    jump_time = jump_time - 1
  else
    collider_move(self, vec(0, 3))
  end
  collider_move(self, v)
end

function key(k)
  if k == KEY_UP and jump_time == 0 then
    jump_time = 40
  end
end

function draw()
  render_sprite(self, sprite, frame_size, frame, 0.0)
end
