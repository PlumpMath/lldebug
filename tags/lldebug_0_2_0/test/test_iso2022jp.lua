--
-- $BF|K\8lI=<((B
--

--require "E:\\programs\\develop\\lldebug\\test\\strict"
--require "strict"

debug = require "debug"
require "table"

function t()
  if 0 == 0 then
    print(debug.traceback("msg"))
    return
  end

  t(i - 1)
end

function g(i)
  while true do
    print(i)
    t()
    i = coroutine.yield()
  end
end

local co = nil
local function f(i)
  if co == nil then
    co = coroutine.create(g)
  end

  local function inner()
    local test = {x = function() end}
	  --print(test)
  end

  local tab = {[0] = "$B%F%9%?!<(B", deeptab = {["x"]=coroutine.create(f)}}
  tab.tt = tab
  coroutine.resume(co, i)
  inner()
end

tab = {[0] = "$B%F%9%H(B", deep = {x=coroutine.create(f)}}
tab.tt = tab
local i = 0
local str = [[$B%F%9%H%G%=(B]]
while 1 do
  print(i, str)
  f(i)
  print(x)
  i = i + 1
end
