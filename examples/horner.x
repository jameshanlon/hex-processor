| Systolic polynomial evaluation by Horner's rule on a pipeline of four cores.
| Each core holds one coefficient of p(x) = x^3 + 2x^2 + 3x + 4 and, as the
| accumulator flows past, applies acc := acc*x + c. With x = 2 (so acc*x is just
| acc + acc) the pipeline computes p(2) = 8 + 8 + 6 + 4 = 26 and prints it.
| Demonstrates a systolic array: a value streaming through stages that each apply
| a fixed step. Coefficients are baked in as literals (par branches take only
| channels).

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

| Leading coefficient (x^3): seeds the accumulator with 1.
proc head(chan out) is out ! 1

| Coefficient 2 (x^2): acc := acc*2 + 2.
proc mid2(chan in, chan out) is
  var acc;
{ in ? acc;
  out ! ((acc + acc) + 2)
}

| Coefficient 3 (x^1): acc := acc*2 + 3.
proc mid3(chan in, chan out) is
  var acc;
{ in ? acc;
  out ! ((acc + acc) + 3)
}

| Constant term (x^0): acc := acc*2 + 4, then print the result.
proc tail(chan in) is
  var acc;
{ in ? acc;
  printn((acc + acc) + 4);
  putval('\n')
}

proc main() is
  chan a;
  chan b;
  chan c;
  par { head(a); mid2(a, b); mid3(b, c); tail(c) }
