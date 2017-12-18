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

CFLAGS      := -Wall -O2 $(EXTRA_CFLAGS) $(INC) -DSOCKLEN_T=socklen_t -DNO_SSTREAM=1 -D_LARGEFILE_SOURCE=1  -D_FILE_OFFSET_BITS=64 -static
CFLAGS		+= -DBSD=1 -DLOCALE_NOT_USED
CFLAGS		+= -DAUDIO_STREAM=1
#CFLAGS		+= -DDEBUG

LDFLAGS     := $(EXTRA_LDFLAGS) -Wl,--as-needed -lpthread

LIVE_LIB 	= live/UsageEnvironment/libUsageEnvironment.a
LIVE_LIB 	+= live/groupsock/libgroupsock.a
LIVE_LIB 	+= live/BasicUsageEnvironment/libBasicUsageEnvironment.a
LIVE_LIB 	+= live/liveMedia/libliveMedia.a


SRC_OBJ	= main.o 
SRC_OBJ	+=ServerMediaSubsession.o
SRC_OBJ	+=H264_V4l2DeviceSource.o V4l2DeviceSource.o
SRC_OBJ	+=AlsaDeviceSource.o
SRC_OBJ	+= sn98600_v4l2.o 
SRC_OBJ	+= sn98600_record_audio.o

#targets =
targets = rtsp_server


.PHONY : clean distclean all
%.o : %.cpp
	$(CPP) -c $(CFLAGS) $<

%.o : %.c
	$(CC) -c $(CFLAGS) $<

all: $(targets)


rtsp_server:  $(SRC_OBJ) $(LIVE_LIB) $(SNX_VC_LIB) $(SNX_RC_LIB) $(SNX_AUDIO_LIB)
	$(CPP) -o $@ $^  $(LDFLAGS)
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
	$(MAKE) -C live $@  INSTALL_DIR=$(abspath $(BIN_INS_DIR))

distclean : clean
	rm -f $(targets)
