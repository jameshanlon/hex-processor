| Binary search over a sorted array. Builds the sorted array {2,4,...,20},
| reads a key byte from stdin, and exits with the index of the key, or 99 if
| it is not present. Complements bubblesort.x (sort then search).

val get = 2;
val length = 10;

array data[length];

var div_x;

func lsu(val x, val y) is
  if (x < 0) = (y < 0)
  then
    return x < y
  else
    return y < 0

func div_step(val b, val y) is
  var r;
{ if (y < 0) or (~lsu(y, div_x))
  then
    r := 0
  else
    r := div_step(b + b, y + y);
  if ~lsu(div_x, y)
  then
  { div_x := div_x - y;
    r := r + b
  }
  else
    skip;
  return r
}

func div(val n, val m) is
{ div_x := n;
  if lsu(n, m)
  then
    return 0
  else
    return div_step(1, m)
}

func search(array a, val n, val key) is
  var lo;
  var hi;
  var mid;
{ lo := 0;
  hi := n - 1;
  while lo <= hi do
  { mid := lo + div(hi - lo, 2);
    if a[mid] = key
    then
      return mid
    else if a[mid] < key
      then
        lo := mid + 1
      else
        hi := mid - 1
  };
  return 99
}

proc main() is
  var i;
{ i := 0;
  while i < length do
  { data[i] := (i + 1) + (i + 1);
    i := i + 1
  };
  0(search(data, length, get(0)))
}
