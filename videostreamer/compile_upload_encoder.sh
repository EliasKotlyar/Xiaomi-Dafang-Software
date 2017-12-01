./compile.sh
HOST=192.168.0.99
ftp-upload -h ${HOST} -u root --password ismart12 -d /system/sdcard/bin/ mJpegStreamer
ftp-upload -h ${HOST} -u root --password ismart12 -d /system/sdcard/bin/ h264streamer
ftp-upload -h ${HOST} -u root --password ismart12 -d /system/sdcard/bin/ jpegSnap
ftp-upload -h ${HOST} -u root --password ismart12 -d /system/sdcard/bin/ h264Snap