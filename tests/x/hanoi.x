| Towers of Hanoi. Reads the number of disks n from stdin and prints the
| sequence of moves to transfer the stack from peg A to peg C using B as the
| spare. Demonstrates recursion that drives output (rather than computing a
| single value).

val put = 1;
val get = 2;

proc putval(val c) is put(c, 0)

proc move(val from, val to) is
{ putval(from);
  putval(' ');
  putval('-');
  putval('>');
  putval(' ');
  putval(to);
  putval('\n')
}

proc hanoi(val n, val from, val to, val via) is
  if n = 0
  then
    skip
  else
  { hanoi(n - 1, from, via, to);
    move(from, to);
    hanoi(n - 1, via, to, from)
  }

proc main() is hanoi(get(0), 'A', 'C', 'B')
