| A producer streams a sequence of characters through a buffer process to a
| consumer that prints them; a zero sentinel marks the end of the stream. Three
| processors connected by two channels. Unlike pipe.x (which relays a single
| value) this relays a whole stream with termination. Prints "ABC".

val put = 1;

proc putval(val c) is put(c, 0)

proc producer(chan out) is
{ out ! 'A';
  out ! 'B';
  out ! 'C';
  out ! 0
}

proc buffer(chan in, chan out) is
  var v;
{ v := 1;
  while v ~= 0 do
  { in ? v;
    out ! v
  }
}

proc consumer(chan in) is
  var v;
{ v := 1;
  while v ~= 0 do
  { in ? v;
    if v ~= 0
    then
      putval(v)
    else
      skip
  }
}

proc main() is
  chan a;
  chan b;
  par { producer(a); buffer(a, b); consumer(b) }
