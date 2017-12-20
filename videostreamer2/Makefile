NAME = snx_rtsp_server

PWD     	:= $(shell pwd)
TARGET      ?= arm-linux
CROSS_COMPILE   ?= $(TARGET)-
CPP     	:= $(CROSS_COMPILE)g++
CC      	:= $(CROSS_COMPILE)g++
AR      	:= $(CROSS_COMPILE)ar
RANLIB      := $(CROSS_COMPILE)ranlib
STRIP 		:= $(CROSS_COMPILE)strip
INSTALL_DIR	?= ../rootfs/usr
BIN_INS_DIR	:= $(INSTALL_DIR)/bin/example
LIB_INS_DIR	:= $(INSTALL_DIR)/lib/example
CATEGORY	:= ipc_func/rtsp_server

MIDDLEWARE_INS_DIR ?= ../live


INC     	= -I$(KERNEL_DIR)/src/include
INC     	+= -I$(MIDDLEWARE_INS_DIR)
INC 		+= -I$(PWD)/include
INC			+= -I $(MIDDLEWARE_INS_DIR)/BasicUsageEnvironment/include
INC			+= -I $(MIDDLEWARE_INS_DIR)/groupsock/include
INC			+= -I $(MIDDLEWARE_INS_DIR)/liveMedia/include
INC			+= -I $(MIDDLEWARE_INS_DIR)/UsageEnvironment/include

## toolchain
EXTRA_LDFLAGS	= 
EXTRA_CFLAGS	+= -I$(AUTOCONF_DIR)

CFLAGS      += -Wall -O2 -I. $(INC) -DSOCKLEN_T=socklen_t -DNO_SSTREAM=1 -D_LARGEFILE_SOURCE=1  -D_FILE_OFFSET_BITS=64
#CFLAGS		+= -DBSD=1 -DLOCALE_NOT_USED
#CFLAGS		+= -DAUDIO_STREAM=1
#CFLAGS		+= -DDEBUG

LDFLAGS     = -L. -muclibc -O2 -lpthread -lm -lrt

LIVE_LIB 	+= $(MIDDLEWARE_INS_DIR)/liveMedia/libliveMedia.a
LIVE_LIB 	+= $(MIDDLEWARE_INS_DIR)/groupsock/libgroupsock.a
LIVE_LIB 	+= $(MIDDLEWARE_INS_DIR)/UsageEnvironment/libUsageEnvironment.a
LIVE_LIB 	+= $(MIDDLEWARE_INS_DIR)/BasicUsageEnvironment/libBasicUsageEnvironment.a
LIVE_LIB 	+= $(MIDDLEWARE_INS_DIR)/BasicUsageEnvironment/libBasicUsageEnvironment.a
LIVE_LIB 	+= $(PWD)/uclibc/libimp.so
LIVE_LIB 	+= $(PWD)/uclibc/libalog.so

SRC_OBJ	= main.o 
SRC_OBJ	+=ServerMediaSubsession.o
SRC_OBJ	+=H264_V4l2DeviceSource.o V4l2DeviceSource.o
SRC_OBJ	+=AlsaDeviceSource.o
SRC_OBJ	+=ImpJpegVideoDeviceSource.o
SRC_OBJ	+=JpegFrameParser.o
SRC_OBJ	+=ImpEncoder.o

#targets =
targets = rtsp_server


.PHONY : clean distclean all
%.o : %.cpp
	$(CPP) -c $(CFLAGS) $<

%.o : %.c
	$(CC) -c $(CFLAGS) $<

all: $(targets)


rtsp_server:  $(SRC_OBJ)
	$(CPP) -o$@  $(LDFLAGS)  $^  $(LIVE_LIB)
	$(STRIP) $@


.PHONY: install
install:
	@ if [ ! -d $(BIN_INS_DIR)/$(CATEGORY) ]; \
	then \
	install -d $(BIN_INS_DIR)/$(CATEGORY); \
	fi
	install -c $(NAME) $(BIN_INS_DIR)/$(CATEGORY)

clean:
	rm -f *.o *.yuv *.bak *.a *.out *.so snx_rtsp_server
	#$(MAKE) -C live $@  INSTALL_DIR=$(abspath $(BIN_INS_DIR))

distclean : clean
	rm -f $(targets)
