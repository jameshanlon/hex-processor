| Concurrent prime sieve (the classic Hoare/occam example). A generator emits
| the integers 2,3,4,... down a pipeline of filter processes terminated by a
| zero sentinel. The first value each filter receives is a prime, which it
| prints; it then forwards only those later values not divisible by that prime.
| With four cores (generator + two filters + a sink filter) this prints the
| first three primes, in order: 2, 3, 5.

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

proc generator(chan out) is
  var i;
{ i := 2;
  while i <= 12 do
  { out ! i;
    i := i + 1
  };
  out ! 0
}

proc filter(chan in, chan out) is
  var p;
  var v;
{ in ? p;
  printn(p);
  putval('\n');
  v := 1;
  while v ~= 0 do
  { in ? v;
    if v = 0
    then
      out ! 0
    else if rem(v, p) ~= 0
      then
        out ! v
      else
        skip
  }
}

proc sink(chan in) is
  var p;
  var v;
{ in ? p;
  printn(p);
  putval('\n');
  v := 1;
  while v ~= 0 do
    in ? v
}

proc main() is
  chan a;
  chan b;
  chan c;
  par { generator(a); filter(a, b); filter(b, c); sink(c) }
