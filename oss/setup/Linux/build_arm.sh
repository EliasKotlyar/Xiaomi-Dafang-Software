#!/bin/sh

# build script for ARM Linux (Nokia's Maemo plattform)

. ./.directories

KERNELDIR=~/maemo_kernel/kernel-source-diablo-2.6.21/kernel-source

rm -rf prototype

mkdir prototype
mkdir prototype/etc
echo "OSSLIBDIR=$OSSLIBDIR" > prototype/etc/oss.conf
mkdir prototype/usr
mkdir prototype/usr/bin
mkdir prototype/usr/sbin
mkdir -p prototype/$OSSLIBDIR
mkdir prototype/$OSSLIBDIR/etc
mkdir prototype/$OSSLIBDIR/conf.tmpl
mkdir prototype/$OSSLIBDIR/lib
mkdir prototype/$OSSLIBDIR/modules

cp target/bin/* prototype/usr/bin
cp target/sbin/* prototype/usr/sbin
cp target/lib/* prototype/$OSSLIBDIR/lib

# Compile the 64 bit div/mod functions required by gcc
rm -f bpabi.s bpabi_s.o bpabi_c.o bpabi.o
cc -c origdir/setup/Linux/arm/bpabi.c -o bpabi_c.o

for n in L_udivsi3 L_idivsi3 L_divsi3 L_aeabi_ldivmod L_aeabi_uldivmod L_dvmd_lnx
do
# Strip position independent code (PLT) from the asm sources before compile
cpp -static -D$n origdir/setup/Linux/arm/lib1funcs.asm | sed 's/..PLT.//g' > $n.s
as -o $n.o $n.s
done

ld -r -o bpabi.o L*.o bpabi_c.o
rm -f L*.s L*.o bpabi_c.o

#build osscore

rm -rf tmp_build
mkdir tmp_build

cp origdir/setup/Linux/arm/Makefile.osscore.arm tmp_build/Makefile
cp origdir/setup/Linux/oss/build/osscore.c tmp_build/osscore.c

cp ./kernel/framework/include/*.h tmp_build/
cp ./kernel/OS/Linux/wrapper/wrap.h tmp_build/
cp ./setup/Linux/oss/build/ossdip.h tmp_build/
cp ./include/soundcard.h tmp_build/
cp ./kernel/framework/include/ossddk/oss_exports.h tmp_build/

if ! (cd tmp_build && make KERNELDIR=$KERNELDIR) > build.log 2>&1
then
   cat build.log
   echo
   echo Building osscore module failed
   exit 1
fi

ld -r tmp_build/osscore.ko target/objects/*.o -o prototype/usr/lib/oss/modules/osscore.ko

if test -f tmp_build/Module.symvers
then
	#Take generated symbol information and add it to module.inc
	echo "static const struct modversion_info ____versions[]" > tmp_build/osscore_symbols.inc
	echo " __attribute__((used))" >> tmp_build/osscore_symbols.inc
	echo "__attribute__((section(\"__versions\"))) = {" >> tmp_build/osscore_symbols.inc
	sed -e "s:^:{:" -e "s:\t:, \":" -e "s:\t\(.\)*:\"},:" < tmp_build/Module.symvers >> tmp_build/osscore_symbols.inc
	echo "};" >> tmp_build/osscore_symbols.inc
else
	echo > tmp_build/osscore_symbols.inc
fi

cp origdir/setup/Linux/oss/build/*.inc tmp_build/

for n in target/modules/*.o
do
	N=`basename $n .o` 

	cp target/build/$N.c tmp_build/$N.c

	sed "s/MODNAME/$N/" < origdir/setup/Linux/arm/Makefile.tmpl.arm > tmp_build/Makefile
	if ! (cd tmp_build && make KERNELDIR=$KERNELDIR) > build.log 2>&1
	then
	   cat build.log
	   echo
	   echo Building $N module failed
	   exit 1
	fi
	ld -r tmp_build/$N.ko bpabi.o target/modules/$N.o -o prototype/usr/lib/oss/modules/$N.ko
done

exit 0
