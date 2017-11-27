/*
 * sample_sinfo.c
 *
 * twe ways to get sensor info
 *
 * 1. open /dev/sinfo; ioctl TOCTL_SINFO_GET
 *
 * 2. echo 1 >/proc/jz/sinfo/info; cat /proc/jz/sinfo/info
 *
 * */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define SENSOR_INFO_IOC_MAGIC  'S'
#define IOCTL_SINFO_GET			_IO(SENSOR_INFO_IOC_MAGIC, 100)
#define IOCTL_SINFO_FLASH		_IO(SENSOR_INFO_IOC_MAGIC, 101)

#define SENSOR_TYPE_INVALID	-1

enum SENSOR_TYPE
{
	SENSOR_TYPE_OV9712=0,
	SENSOR_TYPE_OV9732,
	SENSOR_TYPE_OV9750,
	SENSOR_TYPE_JXH42,
	SENSOR_TYPE_SC1035,
	SENSOR_TYPE_SC1135,
	SENSOR_TYPE_SC1045,
	SENSOR_TYPE_SC1145,
	SENSOR_TYPE_AR0130,
	SENSOR_TYPE_JXH61,
	SENSOR_TYPE_GC1024,
	SENSOR_TYPE_GC1064,
	SENSOR_TYPE_GC2023,
	SENSOR_TYPE_BF3115,
	SENSOR_TYPE_IMX225,
	SENSOR_TYPE_OV2710,
	SENSOR_TYPE_IMX322,
	SENSOR_TYPE_SC2135,
	SENSOR_TYPE_SP1409,
	SENSOR_TYPE_JXH62,
	SENSOR_TYPE_BG0806,
	SENSOR_TYPE_OV4689,
	SENSOR_TYPE_JXF22,
	SENSOR_TYPE_IMX323,
	SENSOR_TYPE_IMX291
};

typedef struct SENSOR_INFO_S
{
	unsigned char *name;
} SENSOR_INFO_T;

SENSOR_INFO_T g_sinfo[] =
{
	{"ov9712"},
	{"ov9732"},
	{"ov9750"},
	{"jxh42"},
	{"sc1035"},
	{"sc1135"},
	{"sc1045"},
	{"sc1145"},
	{"ar0130"},
	{"jxh61"},
	{"gc1024"},
	{"gc1064"},
	{"gc2023"},
	{"bf3115"},
	{"imx225"},
	{"ov2710"},
	{"imx322"},
	{"sc2135"},
	{"sp1409"},
	{"jxh62"},
	{"bg0806"},
	{"ov4689"},
	{"jxf22"},
};
int main(int argc,char **argv)
{
	int ret  = 0;
	int fd   = 0;
	int data = -1;
	/* open device file */
	fd = open("/dev/sinfo", O_RDWR);
	if (-1 == fd) {
		printf("err: open failed\n");
		return -1;
	}
	/* iotcl to get sensor info. */
	/* cmd is IOCTL_SINFO_GET, data note sensor type according to SENSOR_TYPE */

	ret = ioctl(fd,IOCTL_SINFO_GET,&data);
	if (0 != ret) {
		printf("err: ioctl failed\n");
		return ret;
	}
	if (SENSOR_TYPE_INVALID == data)
		printf("##### sensor not found\n");
	else
		printf("##### sensor : %s\n", g_sinfo[data].name);

	/* close device file */
	close(fd);
	return 0;
}
