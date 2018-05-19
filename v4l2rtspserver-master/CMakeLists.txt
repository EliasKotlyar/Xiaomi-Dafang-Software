cmake_minimum_required(VERSION 2.6)

# set project name from current directory
get_filename_component(BASENAME ${CMAKE_SOURCE_DIR} NAME)
project(${BASENAME})

option(COVERAGE "Coverage" OFF)

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall")

find_package(Git)
if(GIT_FOUND)
    EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty OUTPUT_VARIABLE VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    STRING(REGEX REPLACE "^v(.*)" "\\1" VERSION "${VERSION}")
    add_definitions("-DVERSION=\"${VERSION}\"")
endif()


# define executable to build
include_directories("inc")
aux_source_directory(src SRC_FILES)
add_executable(${PROJECT_NAME} ${SRC_FILES})

if (COVERAGE)
	set(COVERAGE_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COVERAGE_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_FLAGS}")
	target_link_libraries(${PROJECT_NAME} gcov)
endif()

# v4l2wrapper
include_directories("v4l2wrapper/inc")
find_library(V4L2WRAPPER_LIBRARY libv4l2wrapper.a PATHS "." NO_CMAKE_FIND_ROOT_PATH)
if (NOT V4L2WRAPPER_LIBRARY)
        message(STATUS "Installing v4l2wrapper")
        EXEC_PROGRAM("git submodule init && git submodule update && make -C v4l2wrapper CC=${CMAKE_C_COMPILER} CFLAGS_EXTRA=--sysroot=${CMAKE_FIND_ROOT_PATH} && cp v4l2wrapper/libv4l2wrapper.a . && make -C v4l2wrapper clean")
	find_library(V4L2WRAPPER_LIBRARY libv4l2wrapper.a PATHS "." NO_CMAKE_FIND_ROOT_PATH)
        message(STATUS "v4l2wrapper built in V4L2WRAPPER_LIBRARY=${V4L2WRAPPER_LIBRARY}")	
endif (NOT V4L2WRAPPER_LIBRARY)
target_link_libraries(${PROJECT_NAME} PUBLIC ${V4L2WRAPPER_LIBRARY})


#pthread
find_package (Threads)
target_link_libraries (${PROJECT_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT})

#ALSA

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_ALSA")
    target_link_libraries (${PROJECT_NAME} PUBLIC ${ALSA_LIBRARY})


# LOG4CPP
find_path(LOG4CPP_INCLUDE_DIR log4cpp/Category.hh)
message("${LOG4CPP_LIBRARY}")
#if (LOG4CPP_INCLUDE_DIR)
#        message(STATUS "Log4Cpp available")
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_LOG4CPP")
#	include_directories(${LOG4CPP_INCLUDE_DIR}) 

#	find_library(LOG4CPP_LIBRARY log4cpp)
#	target_link_libraries(${PROJECT_NAME} "${LOG4CPP_LIBRARY}")
#endif (LOG4CPP_INCLUDE_DIR)


# live555
#find_path(LIBLIVE555_INCLUDE_DIR liveMedia/liveMedia.hh)
#if (NOT LIBLIVE555_INCLUDE_DIR)
#        message(STATUS "Installing liblivemedia-dev")
#        EXEC_PROGRAM("sudo apt-get install -y liblivemedia-dev")
#endif (NOT LIBLIVE555_INCLUDE_DIR)

find_path(LIBLIVE555_INCLUDE_DIR liveMedia/liveMedia.hh)
include_directories(./live/liveMedia/include)
#include_directories(./live/liveMedia/)
include_directories(../live/liveMedia/include)


#target_link_libraries(sharedmem ../v4l2rtspserver-tools/sharedmem.cpp )


include_directories(../v4l2rtspserver-tools/)
add_library(sharedmem ../v4l2rtspserver-tools/sharedmem.cpp)
#add_subdirectory(../v4l2rtspserver-tools/ ./)
target_link_libraries(${PROJECT_NAME} sharedmem)

include_directories("filter")
add_library(filter filter/g711super.c  filter/noise_remover.c  filter/wave.c filter/mylog.c)
target_link_libraries(${PROJECT_NAME} filter)

#include_directories(../live/liveMedia/)
foreach (LIBLIVE555_module groupsock liveMedia UsageEnvironment BasicUsageEnvironment)
	include_directories(../live/${LIBLIVE555_module}/include)

endforeach (LIBLIVE555_module)




set(LIBLIVE555_LIBRARIES "")
foreach (LIBLIVE555_module groupsock liveMedia UsageEnvironment BasicUsageEnvironment)
	find_library( ${LIBLIVE555_module}_LIBRARY ${LIBLIVE555_module} )
	if ( ${LIBLIVE555_module}_LIBRARY )
		set(LIBLIVE555_LIBRARIES ${LIBLIVE555_LIBRARIES} ${${LIBLIVE555_module}_LIBRARY})
	endif ( ${LIBLIVE555_module}_LIBRARY )
endforeach (LIBLIVE555_module)
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../live/liveMedia/libliveMedia.so")
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../live/groupsock/libgroupsock.so")
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../live/UsageEnvironment/libUsageEnvironment.so")
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../live/BasicUsageEnvironment/libBasicUsageEnvironment.so")
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libv4l2wrapper.a")
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/libfilter.a")


target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/uclibc/libalog.so")
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/uclibc/libimp.so")
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/uclibc/libsysutils.so")

target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../lame-3.100/_install/lib/libmp3lame.a")
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../opus/.libs/libopus.so")


#testing
enable_testing()
add_test(help ./${PROJECT_NAME} -h)

# package
install (TARGETS ${PROJECT_NAME} DESTINATION bin)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Michel Promonet") 
SET(CPACK_DEBIAN_PACKAGE_DEPENDS livemedia-utils,liblog4cpp5)
SET(CPACK_PACKAGE_CONTACT "michel.promonet@free.fr") 
SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
find_package(Git)
if(VERSION)
    SET(CPACK_PACKAGE_VERSION "${VERSION}")
endif()
INCLUDE(CPack)

