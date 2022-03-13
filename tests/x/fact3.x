
var mul_x;

func main() is
  return factorial(5)

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

func times(val n, val m) is
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

func factorial(val n) is
  if n = 0
  then
    return 1
  else
    return times(n, factorial(n-1))
