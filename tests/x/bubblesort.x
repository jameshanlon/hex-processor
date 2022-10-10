val length = 4;
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
{ data[0] := 3 
; data[1] := 2 
; data[2] := 1 
; data[3] := 0 
; sort(data, length)
; return check(data, length)
}
