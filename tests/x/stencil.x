| A 1D nearest-neighbour (stencil) exchange on a line of four processors. Each
| core holds one cell of [1, 2, 3, 4] and performs one Jacobi-style update,
| new = left + self + right, with absent neighbours contributing 0. Boundary
| values are exchanged with a rightward then a leftward halo swap (demonstrating
| bidirectional channels). The cells then print their results in order via a
| left-to-right token, giving 3, 6, 9, 7.
|
| To avoid deadlock every core sends before it receives in each phase, and the
| open end of the line starts the matching receive. par branches may only be
| passed channels, so each cell's value is baked in as a literal.

val put = 1;

proc putval(val c) is put(c, 0)

| Leftmost cell, value 1: right neighbour only.
proc head(chan rOut, chan lIn) is
  var self;
  var right;
{ self := 1;
  rOut ! self;          | rightward halo: send self to the right
  lIn ? right;          | leftward halo: receive the right neighbour's value
  self := self + right; | new = 0 + self + right
  putval(self + '0');   | owns the print token first
  putval('\n');
  rOut ! 0              | pass the token right
}

| Interior cell, value 2.
proc mid2(chan rIn, chan rOut, chan lOut, chan lIn) is
  var self;
  var left;
  var right;
  var t;
{ self := 2;
  rOut ! self;          | rightward halo: send self right
  rIn ? left;           | receive the left neighbour's value
  lOut ! self;          | leftward halo: send self left
  lIn ? right;          | receive the right neighbour's value
  self := left + self + right;
  rIn ? t;              | wait for the print token from the left
  putval(self + '0');
  putval('\n');
  rOut ! t              | pass the token right
}

| Interior cell, value 3.
proc mid3(chan rIn, chan rOut, chan lOut, chan lIn) is
  var self;
  var left;
  var right;
  var t;
{ self := 3;
  rOut ! self;
  rIn ? left;
  lOut ! self;
  lIn ? right;
  self := left + self + right;
  rIn ? t;
  putval(self + '0');
  putval('\n');
  rOut ! t
}

| Rightmost cell, value 4: left neighbour only.
proc tail(chan rIn, chan lOut) is
  var self;
  var left;
  var t;
{ self := 4;
  rIn ? left;           | rightward halo: receive the left neighbour's value
  lOut ! self;          | leftward halo: send self left
  self := left + self;  | new = left + self + 0
  rIn ? t;              | wait for the print token
  putval(self + '0');
  putval('\n')
}

proc main() is
  chan r0;
  chan r1;
  chan r2;
  chan l0;
  chan l1;
  chan l2;
  par { head(r0, l0)
      ; mid2(r0, r1, l0, l1)
      ; mid3(r1, r2, l1, l2)
      ; tail(r2, l2)
      }
