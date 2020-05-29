#!/bin/sh

FILE="${1:?"valgrind log file needed as 1st parameter"}"

RANGES="$(awk '
BEGIN {
  skip = 0;
  err = 0;
  count = 0;
}

/==[0-9]*== *.*i965_dri/ {
  if( skip == 0 ) ++count
  skip=1;
}

/==[0-9]*== *[0-9,]* bytes* in [0-9,]* block/ {
  err = NR;
  skip = 0;
}

/==[0-9]*== *Invalid/ {
  err = NR;
  skip = 0;
}

/^==[0-9]*== *$/ {
  if( err != 0 && skip == 1 ) {
    printf "%d,%d d;", err, NR;
  }
  err = 0;
  skip = 0;
}
' "$FILE" )"

sed "$RANGES" "$FILE" | uniq > "fixed_$(basename $FILE)"
