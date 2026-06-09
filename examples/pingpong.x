| Two processors exchanging a value: the pinger sends a value to the ponger,
| which echoes it back, and the pinger prints what returned ('X' = 88).

val put = 1;

proc putval(val c) is put(c, 0)

proc pinger(chan out, chan in) is
  var v;
  { out ! 88;   | send to the ponger
    in ? v;     | receive the echo
    putval(v) } | print it

proc ponger(chan in, chan out) is
  var v;
  { in ? v;     | receive from the pinger
    out ! v }   | echo it back

proc main() is
  chan a;
  chan b;
  par { pinger(a, b); ponger(a, b) }
