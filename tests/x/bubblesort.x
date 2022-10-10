val length = 10;
array data[length];

proc sort(array a, val n) is
  var i;
  var j;
  var tmp;
{ i := 0;
  while i < n do
  { j := 0
  ; while j < (n - (i - 1)) do
    { if a[j] > a[j+1] then
      { tmp := a[j]
      ; a[j] := a[j+1]
      ; a[j+1] := tmp
      }
      else skip
    ; j := j + 1
    }
  ; i := i + 1
  }
}

func check(array a, val n) is
  var i;
{ i := 1
; while i < n do
  { if a[i-1] > a[i] then return 1 else skip
  ; i := i + 1
  }
; return 0
}

proc main() is
{ data[0] := 9
; data[1] := 8
; data[2] := 7
; data[3] := 6
; data[4] := 5
; data[5] := 4
; data[6] := 3
; data[7] := 2
; data[8] := 1
; data[9] := 0
; sort(data, length)
; return check(data, length)
}
