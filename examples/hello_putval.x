val put = 1;

proc main() is
{ putval('h');
  putval('e');
  putval('l');
  putval('l');
  putval('o');
  putval(' ');
  putval('w');
  putval('o');
  putval('r');
  putval('l');
  putval('d');
  newline()
}
proc newline() is putval('\n')

proc putval(val c) is put(c, 0)
