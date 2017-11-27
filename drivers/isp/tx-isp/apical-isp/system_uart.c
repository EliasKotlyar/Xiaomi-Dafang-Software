#include "system_uart.h"
//#include <fcntl.h>

int system_uart_open(void)
{
	//return open("",O_RDWR|O_NONBLOCK);
	return -1;
}

int system_uart_write(int fd, uint8_t* p, int len)
{
	//return write(fd, p, len);
	return 0;
}

int system_uart_read(int fd, uint8_t* p, int len)
{
	//return read(fd, p, len);
	return 0;
}
