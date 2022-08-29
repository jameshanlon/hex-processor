val exit = 0;
val get = 2;

proc main() is
  exit(fib(get(0)))

func fib(val n) is
  if n = 0 then return 0
  else if n = 1 then return 1
  else return fib(n-1) + fib(n-2)
