-- hello.lua ---
#include "foo.lua"
hello(a)
hello(b)
print_vector({5, 9, 10})
v = test()
for index, value in ipairs(v) do print(index, value) end
-- hello("four") -- error
counter = 111
-- function test() counter = counter + 1; hello(counter) end
print("lol")
function foo() error("lol, error") end
function test() foo() end
