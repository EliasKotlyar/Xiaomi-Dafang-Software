/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __APICAL__TYPES_H__
#define __APICAL__TYPES_H__

#include "apical_firmware_config.h"


#if KERNEL_MODULE==1
#include "linux/types.h"
#include "linux/string.h"
//#include "linux/math64.h"
#else
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#endif


#define msleep(x) usleep((x)*1000)
#if KERNEL_MODULE==1
#include <asm/div64.h>
#include <linux/math64.h>
#define usleep(a) usleep_range((a),(a)+100)
#else
#define div64_u64(x,y) ((x)/(y))
#define div64_s64(x,y) ((x)/(y))
#endif

#define UNIT_DISABLED 0
#define UNIT_ENABLED 1

#define array_size(a) \
	(sizeof(a)/sizeof(a[0]))


typedef struct _modulation_entry_t {
	uint16_t x,y;
} modulation_entry_t;

typedef struct _modulation_entry_32_t {
	uint32_t x,y;
} modulation_entry_32_t;


#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct LookupTable {
	void *ptr ;
	uint16_t rows ;
	uint16_t cols ;
	uint16_t width ;
} LookupTable ;
#if ISP_HAS_CONNECTION_SOCKET
enum apical_socket_domain {
	APICAL_SOCKET_DOMAIN_UNIX,
	APICAL_SOCKET_DOMAIN_INET
};

struct apical_socket_f {
	int (* socket)(enum apical_socket_domain sock_domain, int sock_nonblock, int *fd);
	int (* set_reuseaddr)(int fd, int reuseaddr);
	int (* set_nonblock)(int fd, int nonblock);
	int (* bind_port)(int server_fd, unsigned short port);
	int (* bind_path)(int server_fd, const char *path);
	int (* listen)(int fd);
	int (* accept)(enum apical_socket_domain sock_domain, int server_fd, int *client_fd);
	int (* read)(int fd, uint8_t *data, int size, int *have_read);
	int (* write)(int fd, const uint8_t *data, int size, int *have_written);
	int (* close)(int fd);
};
#endif // ISP_HAS_CONNECTION_SOCKET

/*// got to an infinite loop when we try to access NULL memory pointer.
#define CHECK_NULL_PTR( c ) { if ( c == NULL ) LOG(LOG_CRIT, "Trying to access NULL memory pointer") ; while ( 1 ) {} ; }

#define GET_UCHAR_PTR( c ) ((uint8_t *)c->ptr)
#define GET_USHORT_PTR( c ) (( uint16_t *)c->ptr)
#define GET_USHORT_2D_PTR( c ) (( uint16_t (*)[c->cols] )c->ptr )
#define GET_MOD_ENTRY_PTR( c ) ( (modulation_entry_t *)c->ptr )
#define GET_UINT_PTR( c ) ((uint32_t *)c->ptr)
#define GET_ROWS( c ) ( c->rows )
#define GET_COLS( c ) ( c->cols )
#define GET_WIDTH( c ) ( c->width )
#define GET_LEN( c ) ( (c->rows) * (c->cols) )
#define GET_SIZE( c ) ( (c->rows) * (c->cols) * (c->width) )
 */




#endif /* __APICAL__TYPES_H__ */
