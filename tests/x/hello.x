val put = 1;

proc main() is
{ putval('h');
  putval('e');
  putval('l');
  putval('l');
  putval('o');
  newline()
}
proc newline() is putval('\n')

proc putval(val c) is put(c, 0)
