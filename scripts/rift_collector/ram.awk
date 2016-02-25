#!/bin/awk -f
BEGIN{OFS=";"}
/Size/{size=$2} 
/^[ \t]+Locator:/{loc=$2}
/Speed/{speed=$2}
/Manu/{manu=$2}
/Part/{part=$3; print loc,size,speed,manu,part}
