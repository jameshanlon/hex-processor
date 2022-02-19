var x;
var y;
var z;
var foo_BAR;
val c1 = 1 + 2;
val c2 = 1 + 2 + 3;
val c3 = 1 + 2 + (3 + 4);
array b[100];
array c[(1 + 2)];
array d[a + 1];
array e[1 + 2 + 3];
array f[1 + 2 + 3 + 4 + 5];
array g[1 + 2 + 3 + (c1 + c2 + c3)];
array h[1 and 2 and 3 and (c1 or c2 or c3)];

proc baz (val x) is 
{ skip; return 0 + baz(1) + baz() }

proc bar () is 
{ skip; skip; skip; skip; skip }

proc foo ( val a, val b, array c) is
  val x = 1;
  var y;
  { while (y < 1) do y := y + 1
  ; return (0)
  }

proc formal_args ( val f1, array f2, proc f3, proc f4) is skip

proc if_stmt (val a, val b) is
  if a < b then skip else skip
