TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
echo "Using the following Toolchain $TOOLCHAIN"
cd openssl
./Configure linux-generic32 shared -DL_ENDIAN --prefix=${PWD}/_install --openssldir=${PWD}/_install no-async



make clean
make CC=${CROSS_COMPILE}gcc RANLIB=${CROSS_COMPILE}ranlib LD=${CROSS_COMPILE}ld MAKEDEPPROG=${CROSS_COMPILE}gcc PROCESSOR=MIPS
make install