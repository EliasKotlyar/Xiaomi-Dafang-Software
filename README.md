## Compiling Dropbear for Xiaomi-Dafang
1. Get the Cross-compiler:
```
apt-get install linux-libc-dev-mipsel-cross libc6-mipsel-cross libc6-dev-mipsel-cross binutils-mipsel-linux-gnu gcc-mipsel-linux-gnu g++-mipsel-linux-gnu
```


2. Run compile.sh, then transfer the binary "dropbearmulti" to a microsd
3. Mount microsd into the filesystem: 
```
mount /dev/mmcblk0p1 /media/mmc
```
4. Run it in Debugging Mode:
```
./dropbearmulti dropbear -R -F -E -v