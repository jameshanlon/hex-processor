val put = 1;
val bytesperword = 4;

var div_x;
var mul_x;

array wordv[100];
array charv[100];

proc putval(val c) is put(c, 0)

func mul_step(val b, val y) is
  var r;
{ if b < mul_x
  then
    r := mul_step(b + b, y + y)
  else
    r := 0;
  if b <= mul_x
  then
  { mul_x := mul_x - b;
    r := r + y
  }
  else
    skip;
  return r
}

func mul(val n, val m) is
  var y;
{ if n < m
  then
  { mul_x := n;
    y := m
  }
  else
  { mul_x := m;
    y := n
  };
  return mul_step(1, y)
}

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

func rem(val n, val m) is
  var x;
{ x := div(n, m);
  return div_x
}

func mul2(val x, val y) is
  var n;
  var r;
{ r := x;
  n := 1;
  while  n ~= y do
  { r := r + r;
    n := n + n
  };
  return r
}

func exp2(val n) is
  var r;
  var i;
{ i := n;
  r := 1;
  while i > 0 do
  { r := r + r;
    i := i - 1
  };
  return r
}

func packstring(array s, array v) is
  var n;
  var si;
  var vi;
  var w;
  var b;
{ n := s[0];
  si := 0; | input string index
  vi := 0; | output vector index
  b := 0;  | byte index
  w := 0;  | current word
  while si <= n do
  { w :=  w + mul(s[si], exp2(mul2(b, 8)));
    b := b + 1;
    if (b = bytesperword)
    then
    { v[vi] := w;
      vi := vi + 1;
      w := 0;
      b := 0
    }
    else skip;
    si := si + 1
  };
  if (b = 0)
  then
    vi := vi - 1
  else
    v[vi] := w;
  return vi
}

proc unpackstring(array s, array v) is
  var si;
  var vi;
  var b;
  var w;
  var n;
{ si := 0;
  vi := 0;
  b := 0;
  w := s[0];
  n := rem(w, 256);
  while vi <= n do
  { v[vi] := rem(w, 256);
    w := div(w, 256);
    vi := vi + 1;
    b := b + 1;
    if b = bytesperword
    then
    { b := 0;
      si := si + 1;
      w := s[si]
    }
    else skip
  }
}

func strlen(array s) is
{ unpackstring(s, charv);
  return packstring(charv, wordv)
}

proc main() is 0(strlen("xxxxxxxxxxxx"))