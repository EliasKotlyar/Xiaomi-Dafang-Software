
#define RTMP_MODULE_OS

#include "rt_config.h"
#include "rtmp_comm.h"
#include "rt_os_util.h"
#include "rt_os_net.h"

#ifdef RT_CFG80211_SUPPORT
int rt_android_private_command_entry(
	IN      VOID	  *pAdSrc,
	IN      VOID       *pData,
	IN      int        cmd)	   
{
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pAdSrc;

	return 0;
}
#endif /* RT_CFG80211_SUPPORT */
