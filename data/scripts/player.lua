-- Code for the player.
#include "collide.lua"
#include "keys.lua"

local sprite = get_sprite("/tiles/easy.png")
local frame_size = vec(32, 32)
local frame = vec(3, 16)

-- Collider body.
local body = create_body(self, vec(0, 8), vec(6, 16))
set_collide_mask(body, COLLIDE_WORLD)

-- Check bodies.
local CHECK_OVERLAP = 0.01
local down_check = create_body(self, vec(0, 16),
                                     vec(6, CHECK_OVERLAP))
local up_check = create_body(self, vec(0, 0),
                                   vec(6, CHECK_OVERLAP))
local left_check = create_body(self, vec(-3, 8),
                                     vec(CHECK_OVERLAP, 16))
local right_check = create_body(self, vec(3, 8),
                                      vec(CHECK_OVERLAP, 16))
local down_check_prev = false

local GRAVITY = 4
local MOVE_SPEED = 2.5

-- Right now you can up 3 blocks at once.
local JUMP_STAGE_FALL = 0
local JUMP_STAGE_UP = 1
local JUMP_STAGE_CHANGE_UP = 2
local JUMP_STAGE_CAP = 3
local JUMP_STAGE_CHANGE_DOWN = 4
local JUMP_STAGE_TOTAL = 5

local JUMP_PERIOD_UP_MINIMUM = 10
local JUMP_PERIOD_UP = 28
local JUMP_PERIOD_CHANGE = 10
local JUMP_PERIOD_CAP = 5

local JUMP_ALLOWANCE = 4
local JUMP_ALLOWANCE_WALL = 5

local jump_input_buffer = 0
local jump_timer = 0
local jump_stage = JUMP_STAGE_FALL

local wall_jump_timer = 0
local wall_jump_left = false

function update()
  local down_check_this = body_check(self, down_check, COLLIDE_WORLD)
  local up_check_this = body_check(self, up_check, COLLIDE_WORLD)
  local left_check_this = body_check(self, left_check, COLLIDE_WORLD)
  local right_check_this = body_check(self, right_check, COLLIDE_WORLD)
  local left_down = is_key_down(KEY_LEFT)
  local right_down = is_key_down(KEY_RIGHT)

  -- Control wall-jumping.
  if left_check_this and not down_check_this then
    wall_jump_timer = JUMP_ALLOWANCE_WALL
    wall_jump_left = true
  end
  if right_check_this and not down_check_this then
    wall_jump_timer = JUMP_ALLOWANCE_WALL
    wall_jump_left = false
  end
  if down_check_this then
    wall_jump_timer = 0
  end
  if wall_jump_timer > 0 then
    if jump_input_buffer == JUMP_ALLOWANCE and not down_check_this and (
        wall_jump_left and right_down and not left_down or
        not wall_jump_left and left_down and not right_down) then
      jump_stage = JUMP_STAGE_UP
      jump_timer = JUMP_PERIOD_UP
      jump_input_buffer = 0
    end
    wall_jump_timer = wall_jump_timer - 1
  end

  -- Gives a small allowance for pressing jump key before landing.
  if jump_input_buffer > 0 then
    jump_input_buffer = jump_input_buffer - 1
    if jump_stage == JUMP_STAGE_FALL and down_check_this then
      jump_stage = JUMP_STAGE_UP
      jump_timer = JUMP_PERIOD_UP
      jump_input_buffer = 0
    end
  end
  -- Accelerate off cliffs. TODO: fix the wee jerk?
  if jump_stage == JUMP_STAGE_FALL and
      not down_check_this and down_check_prev then
    jump_stage = JUMP_STAGE_CHANGE_DOWN
    jump_timer = JUMP_PERIOD_CHANGE
  end
  -- Stop going upwards when we bash a ceiling.
  if up_check_this and (
      jump_stage == JUMP_STAGE_UP or jump_stage == JUMP_STAGE_CHANGE_UP) then
    if jump_stage == JUMP_STAGE_UP then
      jump_timer = jump_timer + JUMP_PERIOD_CHANGE
    end
    jump_stage = JUMP_STAGE_CAP
    jump_timer = jump_timer + JUMP_PERIOD_CAP
  end

  local v = 0
  if left_down then
    v = v - MOVE_SPEED
  end
  if right_down then
    v = v + MOVE_SPEED
  end

  -- Step up slopes.
  collider_move(self, vec(0, -MOVE_SPEED))
  collider_move(self, vec(v, 0))
  collider_move(self, vec(0, MOVE_SPEED))

  -- Controls minimum jump height.
  if not is_key_down(KEY_UP) and jump_stage == 1 and
      jump_timer < JUMP_PERIOD_UP - JUMP_PERIOD_UP_MINIMUM then
    jump_stage = JUMP_STAGE_CHANGE_UP
    jump_timer = JUMP_PERIOD_CHANGE
  end

  -- Controls jump arc.
  if jump_stage == JUMP_STAGE_UP then
    v = -1
  elseif jump_stage == JUMP_STAGE_CHANGE_UP then
    v = -jump_timer / JUMP_PERIOD_CHANGE
  elseif jump_stage == JUMP_STAGE_CAP then
    v = 0
  elseif jump_stage == JUMP_STAGE_CHANGE_DOWN then
    v = (JUMP_PERIOD_CHANGE - jump_timer) / JUMP_PERIOD_CHANGE
  else
    v = 1
  end

  -- Controls jump sequence.
  if jump_timer > 0 then
    jump_timer = jump_timer - 1
  end
  if jump_timer == 0 and jump_stage ~= JUMP_STAGE_FALL then
    jump_stage = (jump_stage + 1) % JUMP_STAGE_TOTAL
    jump_timer =
        jump_stage == JUMP_STAGE_CHANGE_UP and JUMP_PERIOD_CHANGE or
        jump_stage == JUMP_STAGE_CAP and JUMP_PERIOD_CAP or
        jump_stage == JUMP_STAGE_CHANGE_DOWN and JUMP_PERIOD_CHANGE or 0
  end

  collider_move(self, vec(0, v * GRAVITY))
  down_check_prev = down_check_this
end

function key(k)
  if k == KEY_UP then
    jump_input_buffer = JUMP_ALLOWANCE
  end
end

function draw()
  render_sprite(self, sprite, frame_size, frame, 0.0)
end
