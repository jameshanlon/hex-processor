var mul_x;

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

func main() is 0(mul(2(0), 2(0)))
