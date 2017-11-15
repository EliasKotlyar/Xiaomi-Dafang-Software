## Compiling for Xiaomi Dafang


## Compiling of Dropbear

Run compile_dropbear.sh

## Compiling SFTP-Server

Install tools for buildung:
```
sudo apt-get install autoconf shtool libtool
```
1. Compile zlib using compile_zlib.sh
2. Compile libressl using compile_libressl.sh
3. Compile openssh sftp server using compile_sftp.sh

## Transfering Dropbear:

1. Transfer binary dropbearmulti to sdcard
3. Mount microsd into the filesystem: 
```
mount /dev/mmcblk0p1 /media/mmc
```
4. Run it in Debugging Mode:
```
./dropbearmulti dropbear -R -F -E 

