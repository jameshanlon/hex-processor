| The Ackermann function, a classic example of deep non-primitive recursion.
| Reads two bytes m and n from stdin and exits with ack(m, n). Useful both as
| an example and as a stress test of the compiler's call stack and frames.

val get = 2;

func ack(val m, val n) is
  if m = 0
  then
    return n + 1
  else if n = 0
    then
      return ack(m - 1, 1)
    else
      return ack(m - 1, ack(m, n - 1))

proc main() is 0(ack(get(0), get(0)))
