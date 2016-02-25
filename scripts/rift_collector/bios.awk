#!/bin/awk -f
BEGIN{}
function fields(f1, f2)
{
  r=$f1
  for (i=f1+1; i<=f2; i++) r=sprintf("%s_%s", r, $i);
  return r
}
/Vendor:/ { print "Vendor: "fields(2,NF)}
/Version:/{ print "BIOS_Version: "fields(2,NF)}
/Release Date/ { print "BIOS_Release_date: " $NF}

