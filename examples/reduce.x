| Parallel reduction (sum) over a binary tree of four processors. Two leaves
| send their values inward to be combined in parallel, then the partial sums are
| combined at the root, which prints the total. Holding values 1, 2, 3 and 4 the
| tree computes 1 + 2 + (3 + 4) = 10. Prints "10".
|
| par branches may only be passed channels, so each core's value is baked into
| its process as a literal.

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

| Leaves send their value inward.
proc leafTwo(chan out) is out ! 2
proc leafFour(chan out) is out ! 4

| Inner node holds 3: combine it with the value from leafFour, send to the root.
proc node(chan in, chan out) is
  var x;
{ in ? x;
  out ! (3 + x)
}

| Root holds 1: combine it with leafTwo and the inner partial sum, then print.
proc root(chan inL, chan inR) is
  var x;
  var s;
{ s := 1;
  inL ? x;
  s := s + x;
  inR ? x;
  s := s + x;
  printn(s);
  putval('\n')
}

proc main() is
  chan a;
  chan b;
  chan c;
  par { root(a, c); leafTwo(a); node(b, c); leafFour(b) }
