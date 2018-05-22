./compile_encoder.sh
cp libimp-samples/sample-Encoder-h264 ./encoder
cp libimp-samples/sample-Change-Resolution ./changeresolution
cp libimp-samples/sample-Encoder-jpeg ./jpeg
HOST=192.168.0.99
ftp-upload -h ${HOST} -u root --password ismart12 -d /system/sdcard encoder
ftp-upload -h ${HOST} -u root --password ismart12 -d /system/sdcard jpeg
ftp-upload -h ${HOST} -u root --password ismart12 -d /system/sdcard changeresolution