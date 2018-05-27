/*
 * SU PWM header file.
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co.,Ltd
 * Author: Tiger <bohu.liang@ingenic.com>
 */

#ifndef __SU_PWM_H__
#define __SU_PWM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

typedef unsigned int uint32_t;
/**
 * @file
 * PWM模块头文件
 */

/**
 * @defgroup Sysutils_PWM
 * @ingroup sysutils
 * @brief 脉冲宽度调制
 *
 * PWM的初始化步骤如下：
 * @code
#define PERIOD		300000
#define DUTY		100000
#define POLARITY	1
#define CHN		0
#define TIMEOUT		20

int ret;
SUPWMChnAttr attr;

ret = SU_PWM_Init();
if(ret < 0)
	return ;

attr.period = PERIOD;
attr.duty = DUTY;
attr.polarity = POLARITY;

ret = SU_PWM_CreateChn(CHN, &attr);
if(ret < 0)
	return ;

ret = SU_PWM_EnableChn(CHN);
if(ret < 0)
	return ;

sleep(TIMEOUT);

SU_PWM_DisableChn(CHN);

SU_PWM_DestroyChn(CHN);

SU_PWM_Exit();

return ;

 * @endcode
 * 更多使用方法请参考Samples
 * @{
 */

/**
 * pwm配置结构体
 */
typedef struct {
	int period;	/**< pwm周期，单位ns */
	int duty;	/**< 占空比时间， 单位ns。当polarity = 0时， duty代表低电平占空比时间。当polarity = 1时， duty代表高电平占空比时间。*/
	int polarity;	/**< pwm周期的初始电平. 0 : 初始电平是0，1 : 初始电平是1*/
} SUPWMChnAttr;

/**
 * @fn int SU_PWM_Init(void);
 *
 * 初始化pwm模块
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 使用pwm之前，一定要调用这个函数。
 *
 * @attention 无。
 */
int SU_PWM_Init(void);

/**
 * @fn int SU_PWM_Exit(void);
 *
 * 去初始化pwm模块
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 不使用pwm之后，一定要调用这个函数。
 *
 * @attention 无。
 */
int SU_PWM_Exit(void);

/**
 * @fn int SU_PWM_CreateChn(uint32_t chn_num, SUPWMChnAttr *chn_attr);
 *
 * 创建通道
 *
 * @param[in] chn_num 创建通道的编号
 *
 * @param[in] chn_attr 通道属性指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 创建通道, 可以设置通道的相关属性。
 *
 * @attention 无。
 */
int SU_PWM_CreateChn(uint32_t chn_num, SUPWMChnAttr *chn_attr);

/**
 * @fn SU_PWM_DestroyChn(uint32_t chn_num);
 *
 * 销毁通道
 *
 * @param[in] chn_num 销毁通道的编号
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 在使用这个函数之前，一定要先调用SU_PWM_DisableChn函数.
 */
int SU_PWM_DestroyChn(uint32_t chn_num);

/**
 * @fn int SU_PWM_GetChnAttr(uint32_t chn_num, SUPWMChnAttr *chn_attr);
 *
 * 获得通道属性
 *
 * @param[in] chn_num 获得第几路通道
 *
 * @param[out] chn_attr 通道属性指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 可以获得通道的相关属性，包括：pwm周期, 高电平时间, pwm周期的初始电平, 可以在任何时间调用.
 *
 * @attention 无
 */
int SU_PWM_GetChnAttr(uint32_t chn_num, SUPWMChnAttr *chn_attr);

/**
 * @fn int SU_PWM_SetChnAttr(uint32_t chn_num, SUPWMChnAttr *chn_attr);
 *
 * 设置通道属性
 *
 * @param[in] chn_num 获得第几路通道
 *
 * @param[in] chn_attr 通道属性指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 可以设置通道的相关属性，包括：pwm周期, 高电平时间, pwm周期的初始电平.
 *
 * @attention 必须在通道不工作时调用。
 */
int SU_PWM_SetChnAttr(uint32_t chn_num, SUPWMChnAttr *chn_attr);

/**
 * @fn int SU_PWM_ModifyChnDuty(uint32_t chn_num, int duty);
 *
 * 设置通道属性
 *
 * @param[in] chn_num 获得第几路通道
 *
 * @param[in] chn_attr 通道属性指针
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 用于修改高电平时间.
 *
 * @attention 必须在通道工作时调用。
 */
int SU_PWM_ModifyChnDuty(uint32_t chn_num, int duty);

/**
 * @fn int SU_PWM_EnableChn(uint32_t chn_num);
 *
 * 启动通道
 *
 * @param[in] chn_num 启动第几路通道
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 在使用这个函数之前，必须确保所使能的通道已经创建.
 */
int SU_PWM_EnableChn(uint32_t chn_num);

/**
 * @fn int SU_PWM_DisableChn(uint32_t chn_num);
 *
 * 关闭通道
 *
 * @param[in] chn_num 停止第几路通道
 *
 * @retval 0 成功
 * @retval 非0 失败，返回错误码
 *
 * @remark 无
 *
 * @attention 无
 */
int SU_PWM_DisableChn(uint32_t chn_num);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* __SU_PWM_H__ */
