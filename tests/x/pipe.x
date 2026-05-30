val put = 1;

proc putval(val c) is put(c, 0)

proc source(chan out) is out ! 'P'

proc relay(chan in, chan out) is
  var v;
  { in ? v; out ! v }

proc sink(chan in) is
  var v;
  { in ? v; putval(v) }

proc main() is
  chan a;
  chan b;
  par { source(a); relay(a, b); sink(b) }
