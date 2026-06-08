| Collatz (3n+1) sequence length. Reads a byte n from stdin and exits with the
| number of steps to reach 1. Demonstrates while loops and the div/rem helpers.

val get = 2;

var div_x;

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

func collatz(val n) is
  var count;
{ count := 0;
  while n > 1 do
  { if rem(n, 2) = 0
    then
      n := div(n, 2)
    else
      n := (n + n + n) + 1;
    count := count + 1
  };
  return count
}

proc main() is 0(collatz(get(0)))
