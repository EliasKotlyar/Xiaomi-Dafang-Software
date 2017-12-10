#!/bin/sh

if test -f /etc/oss.conf
then
  . /etc/oss.conf
else
  OSSLIBDIR=/usr/lib/oss
fi

rm -f osscore_mainline.o
ln -s osscore.lib osscore_mainline.o

rm -f Makefile
ln -s Makefile.osscore Makefile

echo Compiling module osscore 

if ! make > compile.list 2>&1
then
  echo Compiling osscore module failed
  cat compile.list
  exit 1
fi

if ! test -d ../modules
then
   mkdir ../modules
fi

if ! test -d ../logs
then
  mkdir ../logs
fi

mv osscore.ko ../modules/
make clean > /dev/null 2>&1

for n in ../objects/*.o
do
	N=`basename $n .o`

	rm -f $N"_mainline.o"
	ln -s $n $N"_mainline.o"

	rm -f Makefile
	sed "s/MODNAME/$N/g" < Makefile.tmpl > Makefile

	echo Compiling module $N

	if ! make > compile.list 2>&1
	then
		echo Compiling module $N failed
		cat compile.list
		exit 2
	fi

	mv $N.ko* ../modules/
	make clean > /dev/null 2>&1
	rm -f Makefile
done

if ! test -f $OSSLIBDIR/etc/installed_drivers
then
   echo "-----------------------------"
   /usr/sbin/ossdetect -v
   echo "-----------------------------"
   echo ""
fi

if test ! -f $OSSLIBDIR/etc/userdefs
then
  echo "autosave_mixer yes" > $OSSLIBDIR/etc/userdefs
fi

exit 0
