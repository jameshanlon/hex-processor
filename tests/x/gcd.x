| Greatest common divisor by Euclid's algorithm. Reads two bytes from stdin
| and exits with their GCD. Demonstrates recursion and the div/rem helpers
| (the X language has no built-in division).

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

func gcd(val a, val b) is
  if b = 0
  then
    return a
  else
    return gcd(b, rem(a, b))

proc main() is 0(gcd(get(0), get(0)))
