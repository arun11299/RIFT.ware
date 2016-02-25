#!/bin/awk -f
function fields(f1, f2)
{
  r=$f1
  for (i=f1+1; i<=f2; i++) r=sprintf("%s_%s", r, $i);
  return r
}

/^CPU\(s\):/{print "CPU_Count: " $NF}
/^Core\(s\) per socket:/{print "Core_Count/Socket: " $NF}
/^Socket\(s\)/{print "Socket_Count: " $NF}
/^Model name:/{print "CPU_Model: " fields(3, NF)}
/NUMA/{print $0}
