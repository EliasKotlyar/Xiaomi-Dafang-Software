LIVEMEDIAPATH = ../live

INCLUDES = -I$(LIVEMEDIAPATH)/UsageEnvironment/include -I$(LIVEMEDIAPATH)/groupsock/include -I$(LIVEMEDIAPATH)/liveMedia/include -I$(LIVEMEDIAPATH)/BasicUsageEnvironment/include -Iinclude/
# Default library filename suffixes for each library that we link with.  The "config.*" file might redefine these later.
libliveMedia_LIB_SUFFIX = $(LIB_SUFFIX)
libBasicUsageEnvironment_LIB_SUFFIX = $(LIB_SUFFIX)
libUsageEnvironment_LIB_SUFFIX = $(LIB_SUFFIX)
libgroupsock_LIB_SUFFIX = $(LIB_SUFFIX)
##### Change the following for your environment:
COMPILE_OPTS =        $(INCLUDES) -I. -O2 -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -DXLOCALE_NOT_USED=1
C =            c
C_COMPILER =        $(CROSS_COMPILE)gcc
CFLAGS +=        $(COMPILE_OPTS)
C_FLAGS = $(CFLAGS)
CPP =            cpp
CPLUSPLUS_COMPILER =    $(CROSS_COMPILE)g++
CPLUSPLUS_FLAGS =    $(COMPILE_OPTS) -Wall -DBSD=1
CPLUSPLUS_FLAGS += $(CPPFLAGS) -fexceptions
OBJ =            o
LINK =            $(CROSS_COMPILE)g++ -o
LINK_OPTS =        -L. $(LDFLAGS)
CONSOLE_LINK_OPTS =    $(LINK_OPTS)
LIBRARY_LINK =        "$(CROSS_COMPILE)ar" "cr" ""

LIBRARY_LINK_OPTS =
LIB_SUFFIX =            a
LIBS_FOR_CONSOLE_APPLICATION = $(CXXLIBS)
LIBS_FOR_GUI_APPLICATION = $(LIBS_FOR_CONSOLE_APPLICATION)
EXE =
##### End of variables to change


PREFIX = /usr/local
ALL = testH264VideoStreamer
all: $(ALL)

extra:	testGSMStreamer$(EXE)

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<
.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<


H264_VIDEO_STREAMER_OBJS = testH264VideoStreamer.$(OBJ) ImpJpegVideoDeviceSource.$(OBJ)  sample-encoder-jpeg.$(OBJ)

USAGE_ENVIRONMENT_DIR = $(LIVEMEDIAPATH)/UsageEnvironment
USAGE_ENVIRONMENT_LIB = $(USAGE_ENVIRONMENT_DIR)/libUsageEnvironment.$(libUsageEnvironment_LIB_SUFFIX)
BASIC_USAGE_ENVIRONMENT_DIR = $(LIVEMEDIAPATH)/BasicUsageEnvironment
BASIC_USAGE_ENVIRONMENT_LIB = $(BASIC_USAGE_ENVIRONMENT_DIR)/libBasicUsageEnvironment.$(libBasicUsageEnvironment_LIB_SUFFIX)
LIVEMEDIA_DIR = $(LIVEMEDIAPATH)/liveMedia
LIVEMEDIA_LIB = $(LIVEMEDIA_DIR)/libliveMedia.$(libliveMedia_LIB_SUFFIX)
GROUPSOCK_DIR = $(LIVEMEDIAPATH)/groupsock
GROUPSOCK_LIB = $(GROUPSOCK_DIR)/libgroupsock.$(libgroupsock_LIB_SUFFIX)
LOCAL_LIBS =	$(LIVEMEDIA_LIB) $(GROUPSOCK_LIB) \
		$(BASIC_USAGE_ENVIRONMENT_LIB) $(USAGE_ENVIRONMENT_LIB)
LIBS =			$(LOCAL_LIBS) $(LIBS_FOR_CONSOLE_APPLICATION)


testH264VideoStreamer$(EXE):	$(H264_VIDEO_STREAMER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(H264_VIDEO_STREAMER_OBJS) $(LIBS)


clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ include/*~

install: $(ALL)
	  install -d $(DESTDIR)$(PREFIX)/bin
	  install -m 755 $(ALL) $(DESTDIR)$(PREFIX)/bin

##### Any additional, platform-specific rules come here:
