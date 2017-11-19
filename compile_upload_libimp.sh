./compile_libimpsamples.sh
cp libimp-samples/sample-Encoder-h264 ./encoder
HOST=192.168.178.22
ftp-upload -h ${HOST} -u root --password ismart12 -d /system/sdcard encoder