#!/usr/bin/bash
case `uname -r` in
 "5.6") OS=solaris6
	REL=6
	;;
 "5.7") OS=solaris7
	REL=7
	;;
 "5.8") OS=solaris8
	REL=8
	;;
 "5.9") OS=solaris9
	REL=9
	;;
 *)	OS=solaris
	REL=10
	;;
esac

ARCH=`uname -p`
PKG=oss
VERSION=`sh $ARCH/showversion.sh`
BUILDID=`cat $ARCH/buildid.dat`
PKGVERSION=$VERSION-$BUILDID
PKGFILE=$PKG-$OS-$PKGVERSION-$ARCH.pkg
# Setup the scripts
echo "CLASSES=none drvcfg" > $ARCH/setup/SunOS/pkginfo
echo "BASEDIR=/" >> $ARCH/setup/SunOS/pkginfo
echo "TZ=PST" >> $ARCH/setup/SunOS/pkginfo
echo "PKG=$PKG" >> $ARCH/setup/SunOS/pkginfo
echo "PATH=/sbin:/usr/sbin:/usr/bin:/usr/sadm/install/bin" >> $ARCH/setup/SunOS/pkginfo
echo "NAME=Open Sound System" >> $ARCH/setup/SunOS/pkginfo
echo "VERSION=$PKGVERSION" >> $ARCH/setup/SunOS/pkginfo
echo "CATEGORY=driver" >> $ARCH/setup/SunOS/pkginfo
echo "DESC=Open Sound System for Solaris $ARCH" >> $ARCH/setup/SunOS/pkginfo
echo "ARCH=$ARCH" >> $ARCH/setup/SunOS/pkginfo
echo "VENDOR=4Front Technologies" >> $ARCH/setup/SunOS/pkginfo
echo "HOTLINE=+1 (310) 202 8530" >> $ARCH/setup/SunOS/pkginfo
echo "EMAIL=support@opensound.com" >> $ARCH/setup/SunOS/pkginfo
echo "VSTOCK=" >> $ARCH/setup/SunOS/pkginfo
echo "PSTAMP=" >> $ARCH/setup/SunOS/pkginfo
echo "PKGINST=oss" >> $ARCH/setup/SunOS/pkginfo
echo "PKGSAV=/var/sadm/pkg/OSS/save" >> $ARCH/setup/SunOS/pkginfo
echo "INSTDATE=" >> $ARCH/setup/SunOS/pkginfo

echo "i pkginfo=$ARCH/setup/SunOS/pkginfo" 		> /tmp/$$
if test -f $ARCH/.date
then
  # Open source version
  echo "i copyright=$ARCH/COPYING"	 		>> /tmp/$$
else
  # Retail version
  echo "i copyright=$ARCH/EULA"		 		>> /tmp/$$
fi
echo "i postinstall=$ARCH/setup/SunOS/postinstall" 	>> /tmp/$$
echo "i preremove=$ARCH/setup/SunOS/preremove" 	>>/tmp/$$
echo "i postremove=$ARCH/setup/SunOS/postremove" 	>>/tmp/$$
echo "i i.drvcfg=$ARCH/setup/SunOS/i.drvcfg" 		>>/tmp/$$
echo "i r.drvcfg=$ARCH/setup/SunOS/r.drvcfg" 		>>/tmp/$$

# now get a list of all the files and directories
(cd prototype; find . -type f -print |pkgproto 		>> /tmp/$$)
(cd prototype; find . -type l -print |pkgproto 		>> /tmp/$$)

# now change the file type of .conf files to editable and set class to drvcfg
EXCEPTIONLIST='.conf|userdefs'

/usr/xpg4/bin/grep -v -E -e $EXCEPTIONLIST /tmp/$$ > proto
/usr/xpg4/bin/grep -E -e $EXCEPTIONLIST /tmp/$$ |sed -e 's/f none/e drvcfg/g' >> proto

# Remove the temp file.
echo "removing /tmp/$$"; rm -f /tmp/$$

#now create the package.
pkgmk -o -d /tmp -r prototype -a $ARCH -f proto
touch $PKGFILE
pkgtrans -s /tmp $PKGFILE $PKG
echo package file is $PKGFILE

if test -f $ARCH/4front-private/export_package.sh
then
  sh $ARCH/4front-private/export_package.sh $PKGFILE $ARCH `sh $ARCH/showversion.sh` /tmp $ARCH-$REL
fi
