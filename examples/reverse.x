| Reverse an array in place. Fills an array with the digits '0'..'n-1', reverses
| it with a two-index swap loop, and prints the result. Reads the length n (a
| small byte, n <= 10) from stdin. Demonstrates arrays and in-place swapping.

val put = 1;
val get = 2;

array a[16];

proc putval(val c) is put(c, 0)

proc main() is
  var n;
  var i;
  var j;
  var tmp;
{ n := get(0);
  | Fill with the characters '0', '1', ...
  i := 0;
  while i < n do
  { a[i] := '0' + i;
    i := i + 1
  };
  | Reverse in place.
  i := 0;
  j := n - 1;
  while i < j do
  { tmp := a[i];
    a[i] := a[j];
    a[j] := tmp;
    i := i + 1;
    j := j - 1
  };
  | Print the reversed array.
  i := 0;
  while i < n do
  { putval(a[i]);
    i := i + 1
  };
  putval('\n')
}
