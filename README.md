## Compiling for Xiaomi Dafang


## Compiling of Dropbear

Run compile_dropbear.sh

## Compiling SFTP-Server

Install tools for buildung:
```
sudo apt-get install autoconf shtool libtool
```



2. Run compile.sh, then transfer the binary "dropbearmulti" to a microsd
3. Mount microsd into the filesystem: 
```
mount /dev/mmcblk0p1 /media/mmc
```
4. Run it in Debugging Mode:
```
./dropbearmulti dropbear -R -F -E 

