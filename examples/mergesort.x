| Parallel merge sort on four processors. A distributor splits an array into two
| halves and sends one to each of two sorter processes; the sorters order their
| pair and stream it (terminated by a 99 sentinel) to a merger, which merges the
| two sorted streams and prints the fully sorted result. Sorting [3,1,4,2] prints
| 1, 2, 3, 4. Demonstrates divide-and-conquer parallelism and a stream merge.

val put = 1;

proc putval(val c) is put(c, 0)

proc distributor(chan toA, chan toB) is
{ toA ! 3;
  toA ! 1;
  toB ! 4;
  toB ! 2
}

| Receive a pair, sort it, and stream it in order followed by a sentinel.
proc sorter(chan in, chan out) is
  var a;
  var b;
  var t;
{ in ? a;
  in ? b;
  if a > b
  then
  { t := a;
    a := b;
    b := t
  }
  else
    skip;
  out ! a;
  out ! b;
  out ! 99
}

| Merge two sorted, sentinel-terminated streams and print each value in order.
proc merger(chan inA, chan inB) is
  var a;
  var b;
{ inA ? a;
  inB ? b;
  while (a ~= 99) or (b ~= 99) do
  { if (b = 99) or ((a ~= 99) and (a <= b))
    then
    { putval(a + '0');
      putval('\n');
      inA ? a
    }
    else
    { putval(b + '0');
      putval('\n');
      inB ? b
    }
  }
}

proc main() is
  chan da;
  chan db;
  chan ma;
  chan mb;
  par { distributor(da, db); sorter(da, ma); sorter(db, mb); merger(ma, mb) }
