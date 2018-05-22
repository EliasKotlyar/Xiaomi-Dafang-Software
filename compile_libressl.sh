TOOLCHAIN=$(pwd)/mips-gcc472-glibc216-64bit/bin
CROSS_COMPILE=$TOOLCHAIN/mips-linux-gnu-

export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export CFLAGS="-muclibc -O2"
export CPPFLAGS="-muclibc -O2"
export LDFLAGS="-muclibc -O2"

if [ ! -d libressl/.git ]
then
  git clone https://github.com/libressl-portable/portable.git
  mv portable/* libressl 
  rmdir portable
fi

cd libressl/
./autogen.sh
./configure --prefix=${PWD}/_install --host=mips-linux-gnu --with-pic
make -j4
make install

# Compile openssl binary by hand, didn't find how to use static lib instead of dynamic
PREF=${PWD}
cd apps/openssl
${CC} -std=gnu99 -muclibc -O2 -Wall -std=gnu99 -fno-strict-aliasing -fno-strict-overflow -D_FORTIFY_SOURCE=2 -fstack-protector-all -DHAVE_GNU_STACK -Wno-pointer-sign -muclibc -O2 -Wl,-z -Wl,relro -Wl,-z -Wl,now -o openssl.nolib apps.o asn1pars.o ca.o ciphers.o crl.o crl2p7.o dgst.o dh.o dhparam.o dsa.o dsaparam.o ec.o ecparam.o enc.o errstr.o gendh.o gendsa.o genpkey.o genrsa.o nseq.o ocsp.o openssl.o passwd.o pkcs12.o pkcs7.o pkcs8.o pkey.o pkeyparam.o pkeyutl.o prime.o rand.o req.o rsa.o rsautl.o s_cb.o s_client.o s_server.o s_socket.o s_time.o sess_id.o smime.o speed.o spkac.o ts.o verify.o version.o x509.o certhash.o apps_posix.o compat/strtonum.o ${PREF}/ssl/.libs/libssl.a ${PREF}/crypto/.libs/libcrypto.a -lpthread -Wl,-rpath -Wl,${PREF}/ssl/.libs -Wl,-rpath -Wl,${PREF}/crypto/.libs -Wl,-rpath -Wl,${PREF}/_install/lib
cp openssl.nolib ${PREF}/_install/bin
cd ../../..
