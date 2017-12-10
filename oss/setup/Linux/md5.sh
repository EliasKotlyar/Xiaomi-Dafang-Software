#!/bin/sh

i=$2
i=${i#.}

# Create the MD5 sums file using the program we have found earlier
case "$1" in
  MD5SUM)
    md5sum ".$i"
  ;;
  MD5)
    x=`md5 ".$i" | awk "{ for (y=1;y<=NF;y++) if ((length(\\$y) == 32) && (\\$y !~ /[\/]/)) {print \\$y; break} }"`; echo "$x  $i"
  ;;
  DIGEST)
    x=`digest -a md5 ".$i"`; echo "$x  $i"
  ;;
  OPENSSL)
    x=`openssl md5 $i | awk "{ for (y=1;y<=NF;y++) if ((length(\\$y) == 32) && (\\$y !~ /[\/]/)) {print \\$y; break} }"`; echo "$x  $i"
  ;;
esac
