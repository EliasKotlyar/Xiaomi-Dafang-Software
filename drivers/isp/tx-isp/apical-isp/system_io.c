#include <asm/io.h>
#include <apical-isp/system_io.h>
#include "tx-isp-core.h"
#include <apical-isp/apical_ext_config.h>

uint8_t apical_ext_sytem_mem[APICAL_EXT_SYSTEM_ADDR_MAX-APICAL_EXT_SYSTEM_ADDR_MIN+1];
static void __iomem *apical_io_base;

void system_isp_set_base_address(void *address)
{
	apical_io_base = address;
}

uint32_t system_isp_read_32(uint32_t addr)
{
	return tx_isp_readl(apical_io_base, addr);
}

uint16_t system_isp_read_16(uint32_t addr)
{
	return tx_isp_readw(apical_io_base, addr);
}

uint8_t  system_isp_read_8(uint32_t addr)
{
	return tx_isp_readb(apical_io_base, addr);
}

void system_isp_write_32(uint32_t addr, uint32_t data)
{
	/* if(addr >= 0x540 && addr <= 0x590 && addr != 0x570) */
	/* 	return; */
	tx_isp_writel(apical_io_base, addr, data);
}

void system_isp_write_16(uint32_t addr, uint16_t data)
{
	tx_isp_writew(apical_io_base, addr, data);
}

void system_isp_write_8( uint32_t addr, uint8_t  data)
{
	tx_isp_writeb(apical_io_base, addr, data);
}

