| A token ring of three processors. The starter injects a token which is
| passed around the ring by two reused forwarder processes and returns to the
| starter, which prints it ('Z' = 90). Demonstrates proc reuse: the same
| forwarder runs on two processors with different channel arguments.

val put = 1;

proc putval(val c) is put(c, 0)

proc starter(chan out, chan in) is
  var v;
  { out ! 90;   | inject the token
    in ? v;     | wait for it to return around the ring
    putval(v) } | print it

proc forwarder(chan in, chan out) is
  var v;
  { in ? v;     | receive from the previous node
    out ! v }   | pass to the next node

proc main() is
  chan a;
  chan b;
  chan c;
  par { starter(a, c); forwarder(a, b); forwarder(b, c) }
