/*
 * Purpose: OSS version ID
 */
#include <buildid.h>
#include <timestamp.h>

#ifndef OSS_LICENSE
#define OSS_LICENSE "" /* Empty means commercial license */
#endif 

#define OSS_VERSION_ID "4.2"
#define OSS_VERSION_STRING OSS_VERSION_ID " (b " OSS_BUILD_ID "/" OSS_COMPILE_DATE ")"
#define OSS_INTERNAL_VERSION 0x040199
