#include "apical_isp_io.h"
#include "tx-isp-core.h"
unsigned int watchaddr[50] = {
	/*	0x00010, 0x00014, 0x00040, 0x00044,
		0x00b00, 0x00b04, 0x009c0, 0x009c4,
		0x00130, 0x00b1c, 0x00b20, 0x00b24,
		0x00080, 0x00084, 0x00088, 0x0008c,
		0x00090, 0x00094, 0x00098, 0x0009c,
	 */	0x00b08, 0x00b0c, 0x00b10, 0x00b14,
	0x00b18,
	0,
};
static int trap_addr(unsigned int addr)
{
	int ret = 0;
	int i = 0;
	for(i = 0; i < 50; i++)
	{
		if(watchaddr[i] == 0){
			break;
		}
		if(watchaddr[i] == addr){
			ret = 1;
			break;
		}
	}
	return ret;
}
static void __iomem *apical_io_base;
uint32_t APICAL_READ_32(uint32_t addr)
{
	return tx_isp_readl(apical_io_base, addr);
}
uint16_t APICAL_READ_16(uint32_t addr)
{
	return tx_isp_readw(apical_io_base, addr);
}
uint8_t APICAL_READ_8(uint32_t addr)
{
	return tx_isp_readb(apical_io_base, addr);
}
void APICAL_WRITE_32(uint32_t addr, uint32_t data)
{
	//	if(addr == 0x544)
	//		dump_stack();
	tx_isp_writel(apical_io_base, addr, data);
}
void APICAL_WRITE_16(uint32_t addr, uint16_t data)
{
	tx_isp_writew(apical_io_base, addr, data);
}
void APICAL_WRITE_8(uint32_t addr, uint8_t data)
{
	tx_isp_writeb(apical_io_base, addr, data);
}
void apical_iobase_init(void __iomem *base)
{
	apical_io_base = base;
}
