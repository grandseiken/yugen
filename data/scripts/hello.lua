-- hello.lua ---
a, b = twice(3)
hello(a)
hello(b)
-- hello("four") -- error
counter = 111
-- function test() counter = counter + 1; hello(counter) end
print("lol")
function foo() error("lol, error") end
function test() foo() end
