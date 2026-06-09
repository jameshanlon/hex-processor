func mul2(val x, val y) is
  var n;
  var r;
{ r := x;
  n := 1;
  while  n ~= y do
  { r := r + r;
    n := n + n
  };
  return r
}

proc main() is
  0(mul2(2(0), 2(0)))
