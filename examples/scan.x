| Parallel prefix sum (scan) along a line of four processors holding [1,2,3,4].
| Each core receives the running sum from its left neighbour, adds its own value
| to obtain its prefix, prints it, and passes the running sum on. The strict
| left-to-right flow both computes and orders the output: 1, 3, 6, 10. This is
| the scan counterpart to the tree reduction in reduce.x. Values are baked in as
| literals because par branches may only be passed channels.

val put = 1;

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

proc printn(val n) is
{ if n > 9
  then
    printn(div(n, 10))
  else
    skip;
  putval(rem(n, 10) + '0')
}

| Leftmost cell, value 1: starts the running sum.
proc head(chan out) is
{ printn(1);
  putval('\n');
  out ! 1
}

| Interior cell, value 2.
proc mid2(chan in, chan out) is
  var s;
{ in ? s;
  s := s + 2;
  printn(s);
  putval('\n');
  out ! s
}

| Interior cell, value 3.
proc mid3(chan in, chan out) is
  var s;
{ in ? s;
  s := s + 3;
  printn(s);
  putval('\n');
  out ! s
}

| Rightmost cell, value 4: ends the scan.
proc tail(chan in) is
  var s;
{ in ? s;
  s := s + 4;
  printn(s);
  putval('\n')
}

proc main() is
  chan a;
  chan b;
  chan c;
  par { head(a); mid2(a, b); mid3(b, c); tail(c) }
