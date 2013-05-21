-- hello.lua --
local sprite = get_sprite("/tiles/ruin.png")
local body = create_body(self, vec(0, 0), vec(32, 32))

function update()
  collider_move(self, vec(0, 2.17))
end
function draw()
  render_sprite(self, sprite, vec(32, 32), vec(4, 3), 0.0)
end
