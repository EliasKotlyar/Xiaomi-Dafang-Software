INCLUDES = -I../UsageEnvironment/include -I../groupsock/include -I../liveMedia/include -I../BasicUsageEnvironment/include
##### Change the following for your environment: 
CROSS_COMPILE=        $(PWD)/../../mips-gcc472-glibc216-64bit/bin/mips-linux-gnu-
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

ALL = ElphelStreamer$(EXE)
all:	$(ALL)

misc:	testMCT$(EXE) testGSMStreamer$(EXE)

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $<       

.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $<

ELPHEL_STREAMER_OBJS = ElphelJPEGDeviceSource.$(OBJ) ElphelStreamer.$(OBJ)

USAGE_ENVIRONMENT_DIR = ../UsageEnvironment
USAGE_ENVIRONMENT_LIB = $(USAGE_ENVIRONMENT_DIR)/libUsageEnvironment.$(LIB_SUFFIX)
BASIC_USAGE_ENVIRONMENT_DIR = ../BasicUsageEnvironment
BASIC_USAGE_ENVIRONMENT_LIB = $(BASIC_USAGE_ENVIRONMENT_DIR)/libBasicUsageEnvironment.$(LIB_SUFFIX)
LIVEMEDIA_DIR = ../liveMedia
LIVEMEDIA_LIB = $(LIVEMEDIA_DIR)/libliveMedia.$(LIB_SUFFIX)
GROUPSOCK_DIR = ../groupsock
GROUPSOCK_LIB = $(GROUPSOCK_DIR)/libgroupsock.$(LIB_SUFFIX)
LOCAL_LIBS =	$(LIVEMEDIA_LIB) $(GROUPSOCK_LIB) \
		$(USAGE_ENVIRONMENT_LIB) $(BASIC_USAGE_ENVIRONMENT_LIB)
LIBS =			$(LOCAL_LIBS) $(LIBS_FOR_CONSOLE_APPLICATION)

ElphelStreamer$(EXE):	$(ELPHEL_STREAMER_OBJS) $(LOCAL_LIBS)
	$(LINK)$@ $(CONSOLE_LINK_OPTS) $(ELPHEL_STREAMER_OBJS) $(LIBS)

clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ include/*~

##### Any additional, platform-specific rules come here:

