#!/bin/bash

logfile="PREALLOC/os/linux/Module.symvers"

if [ -e $logfile ]; then
  cat PREALLOC/os/linux/Module.symvers >> os/linux/Module.symvers
fi
echo

exit 0
