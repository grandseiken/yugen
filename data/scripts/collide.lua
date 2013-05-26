-- Collision type enums.
-- Reserved values must be kept in sync with collision.cpp.

-- Reserved values.
local COLLIDE_NONE = 0
local COLLIDE_WORLD = 1
local COLLIDE_RESV0 = 2
local COLLIDE_RESV1 = 4
local COLLIDE_RESV2 = 8

-- Custom values.

-- Standard overlap size for check bodies.
local CHECK_OVERLAP = 0.01

-- Create standard check bodies.
local function create_up_check(script, offset, size)
  return create_body(script, offset - vec(0, size:y() / 2),
                     vec(size:x(), CHECK_OVERLAP))
end

local function create_down_check(script, offset, size)
  return create_body(script, offset + vec(0, size:y() / 2),
                     vec(size:x(), CHECK_OVERLAP))
end

local function create_left_check(script, offset, size)
  return create_body(script, offset - vec(size:x() / 2, 0),
                     vec(CHECK_OVERLAP, size:y()))
end

local function create_right_check(script, offset, size)
  return create_body(script, offset + vec(size:x() / 2, 0),
                     vec(CHECK_OVERLAP, size:y()))
end

local function create_all_checks(script, offset, size)
  return
      create_up_check(script, offset, size),
      create_down_check(script, offset, size),
      create_left_check(script, offset, size),
      create_right_check(script, offset, size)
end
