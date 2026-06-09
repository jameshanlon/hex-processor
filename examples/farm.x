| A worker farm: a distributor scatters tasks to two workers, which double each
| value and forward the results to a collector that sums and prints them. Four
| cores wired so the distributor has two output channels and the collector two
| input channels (fan-out / fan-in). Prints "200".

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

proc distributor(chan toA, chan toB) is
{ toA ! 10;
  toA ! 20;
  toB ! 30;
  toB ! 40
}

proc worker(chan in, chan out) is
  var x;
{ in ? x;
  out ! (x + x);
  in ? x;
  out ! (x + x)
}

proc collector(chan inA, chan inB) is
  var s;
  var x;
{ s := 0;
  inA ? x;
  s := s + x;
  inB ? x;
  s := s + x;
  inA ? x;
  s := s + x;
  inB ? x;
  s := s + x;
  printn(s);
  putval('\n')
}

proc main() is
  chan da;
  chan db;
  chan ca;
  chan cb;
  par { distributor(da, db); worker(da, ca); worker(db, cb); collector(ca, cb) }
