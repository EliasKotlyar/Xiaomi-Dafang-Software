./compile.sh
HOST=192.168.0.99
ftp-upload -h ${HOST} -u root --password ismart12 -d /system/sdcard/bin/ USBMissileLauncherUtils
ftp-upload -h ${HOST} -u root --password ismart12 -d /system/sdcard/bin/ libusb-1.0.so.0
ftp-upload -h ${HOST} -u root --password ismart12 -d /system/sdcard/bin/ libusb-0.1.so.4