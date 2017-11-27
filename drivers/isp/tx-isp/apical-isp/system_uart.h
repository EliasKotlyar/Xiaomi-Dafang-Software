#ifndef __SYSTEM_UART_H__
#define __SYSTEM_UART_H__

#include <apical-isp/apical.h>

int system_uart_open(void);
int system_uart_write(int fd, uint8_t* p, int len);
int system_uart_read(int fd, uint8_t* p, int len);

#endif
