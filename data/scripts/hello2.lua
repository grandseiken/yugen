-- A thing that is made by the test thing.

local sprite = get_sprite("/tiles/temple.png")
local script = get_script("/scripts/hello.lua")
local counter = 0
math.randomseed(os.time() + get_origin(self):x() + get_origin(self):y() * 17)
local r = math.floor(100 * math.random())
function update()
  local o = vec(1, -1) + get_origin(self)
  set_origin(self, o)
  counter = counter + 1
  if counter == r then
  -- create(script, o)
  end
  if counter == 80 then
    destroy(self)
  end
end
function draw()
  render_sprite(self, sprite, vec(32, 32), vec(1, 0), 0.0)
end
