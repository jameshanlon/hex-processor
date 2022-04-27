val put = 1;

proc putval(val c) is put(c, 0)

proc newline() is putval('\n')

proc main() is
{
  putval('h');
  putval('e');
  putval('l');
  putval('l');
  putval('o');
  newline()
}

