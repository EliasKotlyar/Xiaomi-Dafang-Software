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
```


## Compiling Ftp
Run compile_bftpd.sh


## Running FTP:

Copy bftpd and bftpd.conf to /system/dafang/ on the Camera
Run bftpd:
```
/system/dafang/bftpd -d
```

## Compiling the Encoder:


Run compile_libimpsamples.sh

## Getting a H264 Video:

1. Transfer the encoder to Camera
2. Run the following Commands for initialisation:
```
insmod /driver/sensor_jxf22.ko data_interface=2 pwdn_gpio=-1 reset_gpio=18 sensor_gpio_func=0

```
3. Run encoder binary

Sample Output:
```
[root@Ingenic-uc1_1:sdcard]# ./encoder 
sample_system_init start
[ 4815.181111] set sensor gpio as PA-low-10bit
[ 4815.213962] jxf22 0-0040: jxf22 chip found @ 0x40 (i2c0)
[ 4815.219472] tx_isp: Registered sensor subdevice jxf22 0-0040
ImpSystemInit success
i264e[info]: profile Main, level 3.0
[ 4815.793904] &&& chan2  scaler.max_width = 800 max_height = 800  min_width = 128 min_height = 128 &&&
[chn1] scaler->outwidth = 640 scaler->outheight = 360, sscaler.outwidth = 640 sscaler.outheight = 360
Open Stream file /tmp/stream-1.h264 OK
i264e[info]: kb/s:132.72
sample_system_exit start
 sample_system_exit success


```

Trick to increase RAM:
```


echo 100 > /proc/sys/vm/swappiness
echo 16777216 > /sys/block/zram0/disksize
mkswap /dev/zram0
swapon /dev/zram0

```