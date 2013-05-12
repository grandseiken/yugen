-- hello.lua --
function update()
  origin = get_origin(self)
  set_origin(self, vec(1, 1) + origin)
end
function draw()
  render_sprite(self, "/yedit/missing.png", vec(32, 32), vec(0, 0), 0.0)
end
