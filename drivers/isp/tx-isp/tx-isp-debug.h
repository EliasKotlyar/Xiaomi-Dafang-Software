#ifndef _TX_ISP_DEBUG_H_
#define _TX_ISP_DEBUG_H_

/* =================== switchs ================== */

/**
 * default debug level, if just switch ISP_WARNING
 * or ISP_INFO, this not effect DEBUG_REWRITE and
 * DEBUG_TIME_WRITE/READ
 **/
//#define OVISP_CSI_TEST
#define PRINT_LEVEL		ISP_WARNING_LEVEL
//#define PRINT_LEVEL		ISP_INFO_LEVEL
/* =================== print tools ================== */

#define ISP_INFO_LEVEL		0x0
#define ISP_WARNING_LEVEL	0x1
#define ISP_ERROR_LEVEL		0x2
#define ISP_PRINT(level, format, ...)		\
	isp_printf(level, format, ##__VA_ARGS__)
#define ISP_DEBUG(...) ISP_PRINT(ISP_INFO, __VA_ARGS__)

//extern unsigned int isp_print_level;
int isp_debug_init(void);
int isp_debug_deinit(void);
int isp_printf(unsigned int level, unsigned char *fmt, ...);
#endif /* _ISP_DEBUG_H_ */
