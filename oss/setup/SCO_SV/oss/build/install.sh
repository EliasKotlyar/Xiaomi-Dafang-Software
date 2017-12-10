#!/bin/sh

. /etc/oss.conf

if test "$CONFDIR " = " "
then
  CONFDIR=/etc/conf
fi

# Remove the non-oss ICH driver before doing anything else
/etc/conf/bin/idinstall -R $CONFDIR -d ich > /dev/null 2>&1

# Unload previous modules
for n in `ls $OSSLIBDIR/modules|egrep -v "osscore|oss_imux"`
do
  modadmin -U $n > /dev/null 2>&1
done

for MOD in osscore oss_imux
do
  modadmin -U $MOD > /dev/null 2>&1
done

# (re)install osscore and oss_imux

rm -f $OSSLIBDIR/modules/*/install.log

for MOD in osscore oss_imux
do
  cd $OSSLIBDIR/modules/$MOD

  if test ! -f Space.c
  then
     cp $OSSLIBDIR/space.inst/$MOD Space.c
  fi

  rm -f install.log
  
  if /etc/conf/bin/idinstall -k -P oss -R $CONFDIR -M $MOD >> install.log 2>&1
  then
    if /etc/conf/bin/idbuild -M $MOD >> install.log 2>&1
    then
      echo OSS module $MOD installed OK
      echo OSS module $MOD installed OK >> install.log
    else
      cat install.log
      echo Building $MOD module failed
      echo Building $MOD module failed >> install.log
      exit 1
    fi
  else
    cat install.log
    echo Failed to idinstall $MOD
    echo Failed to idinstall $MOD >> install.log
    exit 1
  fi

  (cd $OSSLIBDIR/conf && rm -f $MOD.conf && ln -sf ../modules/$MOD/Space.c $MOD.conf)

  installf oss $CONFDIR/sdevice.d/$MOD
done


# Only install the drivers we have a resmgr match for
OSSTMPFILE=/tmp/ossdetect.$$
CDIR=`pwd`
cd $OSSLIBDIR/modules
/sbin/resmgr -p BRDID > $OSSTMPFILE
for file in */Drvmap
do
	DRVR=`dirname $file`
	grep "^|" $file | cut -d \| -f3 | while read BRDID
	do
		if [ -n "$BRDID" ]; then
			while read RMBRDID
			do
				if [ "$BRDID" = "$RMBRDID" ]; then
					echo "$DRVR"
				fi
			done < $OSSTMPFILE
		fi
	done
done | sort -u | while read DRVR
do
  cd $OSSLIBDIR/modules/$DRVR

  if test ! -f Space.c
  then
     cp $OSSLIBDIR/../space.inst/$n Space.c
  fi
 
  rm -f install.log

# /etc/conf/bin/idinstall -R $CONFDIR -d $DRVR > /dev/null 2>&1

  if /etc/conf/bin/idinstall -k -P oss -R $CONFDIR -M $DRVR >> install.log 2>&1
  then
    if /etc/conf/bin/idbuild -M $DRVR >> install.log 2>&1
    then
      echo OSS module $DRVR installed OK
      echo OSS module $DRVR installed OK >> install.log
    else
      cat install.log
      echo Building $DRVR module failed
      echo Building $DRVR module failed >> install.log
      exit 1
    fi
  else
    cat install.log
    echo Failed to idinstall $DRVR
    echo Failed to idinstall $DRVR >> install.log
    exit 1
  fi
  installf oss $CONFDIR/sdevice.d/$DRVR

  (cd $OSSLIBDIR/conf && rm -f $DRVR.conf && \
	 ln -sf ../modules/$DRVR/Space.c $DRVR.conf)
done

rm -f $OSSTMPFILE

cd $CDIR

if test -f /bin/dcu
then
  /bin/dcu -S
else
  /sbin/dcu -S
fi

if test ! -f $OSSLIBDIR/etc/installed_drivers
then
echo "----------------------------------------------"
        /usr/sbin/ossdetect -v
echo "----------------------------------------------"
fi
echo OSS modules installed OK
