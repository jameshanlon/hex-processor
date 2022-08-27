val put = 1;
val bytesperword = 4;
var div_x;

proc putval(val c) is put(c, 0)

func lsu(val x, val y) is
  if (x < 0) = (y < 0)
  then
    return x < y
  else
    return y < 0

func div_step(val b, val y) is
  var r;
{ if (y < 0) or (~lsu(y, div_x))
  then
    r := 0
  else
    r := div_step(b + b, y + y);
  if ~lsu(div_x, y)
  then
  { div_x := div_x - y;
    r := r + b
  }
  else
    skip;
  return r
}

func div(val n, val m) is
{ div_x := n;
  if lsu(n, m)
  then
    return 0
  else
    return div_step(1, m)
}

func rem(val n, val m) is
  var x;
{ x := div(n, m);
  return div_x
}

proc prints(array s) is
  var n;
  var p;
  var w;
  var l;
  var b;
{ n := 1;
  p := 0;
  w := s[p];
  l := rem(w, 256);
  w := div(w, 256);
  b := 1;
  while (n <= l) do
  { putval(rem(w, 256));
    w := div(w, 256);
    n := n + 1;
    b := b + 1;
    if (b = bytesperword)
    then
    { b := 0;
      p := p + 1;
      w := s[p]
    }
    else skip
  }
}

proc main() is prints("x")
