TOOLCHAIN=$(pwd)/../mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-
echo "Using the following Toolchain $TOOLCHAIN"
make clean
export CC=${CROSS_COMPILE}gcc
export CFLAGS="-muclibc -O2"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"

LIBUSB_PATH=${PWD}/../libusb-1.0.21/_install/
export LIBUSB_1_0_CFLAGS=-I$LIBUSB_PATH/include/libusb-1.0
export LIBUSB_1_0_LIBS="-L$LIBUSB_PATH"

./configure --host=mips-linux-gnu --prefix=${PWD}/_install
make
make install

