| Print every prime less than n (read as a byte from stdin), one per line, by
| trial division. Demonstrates nested loops, early return and the div/rem and
| printn helpers.

val put = 1;
val get = 2;

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

func isprime(val n) is
  var i;
{ if n < 2
  then
    return 0
  else
    skip;
  i := 2;
  while i < n do
  { if rem(n, i) = 0
    then
      return 0
    else
      skip;
    i := i + 1
  };
  return 1
}

proc main() is
  var n;
  var i;
{ n := get(0);
  i := 2;
  while i < n do
  { if isprime(i) = 1
    then
    { printn(i);
      putval('\n')
    }
    else
      skip;
    i := i + 1
  }
}
