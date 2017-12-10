#!/bin/sh

DIR=`dirname $0`

if test "$DIR " = " "
then
  DIR=.
fi

grep "define OSS_VERSION_ID" $DIR/kernel/framework/include/oss_version.h|sed 's/.*_ID "/v/'|sed 's/"//'
