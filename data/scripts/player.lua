-- hello.lua --
local sprite = get_sprite("/tiles/easy.png")
local frame_size = vec(32, 32)
local frame = vec(3, 16)
local body = create_body(self, vec(0, 8), vec(6, 16))

function update()
  -- collider_move(self, vec(0, 3))
end
function draw()
  render_sprite(self, sprite, frame_size, frame, 0.0)
end
