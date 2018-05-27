/*
 * Ingenic IMP PWM solution.
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co.,Ltd
 * Author: Tiger <bohu.liang@ingenic.com>
 */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "pwm.h"

#define PATH_PWM "/dev/pwm"

#define PWM_CONFIG	0x001
#define PWM_CONFIG_DUTY	0x002
#define PWM_ENABLE	0x010
#define PWM_DISABLE 0x100

#define NR_MAX_PWM_NUM 8

struct pwm_ioctl_t {
	int index;
	int duty;
	int period;
	int polarity;
};

typedef struct {
	int fd;
	int create[NR_MAX_PWM_NUM];
	int enable[NR_MAX_PWM_NUM];
	struct pwm_ioctl_t *attr[NR_MAX_PWM_NUM];
}SUPWM;

static SUPWM *gpwm = NULL;

static SUPWM *getpwm(void)
{
	return gpwm;
}

static void setpwm(SUPWM *pwm)
{
	gpwm = pwm;
}

int SU_PWM_Init(void)
{
	SUPWM *pwm;
	int i, ret = 0;

	pwm = calloc(1, sizeof(SUPWM));
	if(pwm == NULL) {
		printf("calloc SUPWM error !\n");
		ret = -1;
		goto err_calloc_pwm;
	}

	pwm->fd = open(PATH_PWM, O_RDONLY);
	if(pwm->fd < 0) {
		printf("open() error !\n");
		ret = -1;
		goto err_open_pwm;
	}

	for(i = 0; i < NR_MAX_PWM_NUM; i++) {
		pwm->attr[i] = calloc(1, sizeof(struct pwm_ioctl_t));
		if(pwm->attr[i] == NULL) {
			printf("calloc attr error !\n");
			ret = -1;
			goto err_calloc_attr;
		}
	}

	setpwm(pwm);

	return 0;

err_calloc_attr:
	close(pwm->fd);
err_open_pwm:
	free(pwm);
err_calloc_pwm:
	return ret;
}

int SU_PWM_Exit(void)
{
	int i;
	SUPWM *pwm = getpwm();

	if(pwm == NULL)
		return 0;

	for(i = 0; i < NR_MAX_PWM_NUM; i++)
		free(pwm->attr[i]);

	close(pwm->fd);

	free(pwm);

	return 0;
}

int SU_PWM_CreateChn(uint32_t chn_num, SUPWMChnAttr *chn_attr)
{
	SUPWM *pwm = getpwm();

	if(pwm == NULL) {
		printf("pwm is NULL !\n");
		return -1;
	}

	if((chn_num >= NR_MAX_PWM_NUM) || (chn_num < 0)) {
		printf("chn_num error !\n");
		return -1;
	}

	if((chn_attr->period < 200) || (chn_attr->period > 1000000000)) {
		printf("chn_attr->period error !\n");
		return -1;
	}

	if((chn_attr->duty < 0) || (chn_attr->duty > chn_attr->period)) {
		printf("chn_attr->duty error !\n");
		return -1;
	}

	if((chn_attr->polarity < 0) || (chn_attr->polarity > 1)) {
		printf("chn_attr->polarity error !\n");
		return -1;
	}

	pwm->attr[chn_num]->index = chn_num;
	pwm->attr[chn_num]->period = chn_attr->period;
	pwm->attr[chn_num]->duty = chn_attr->duty;
	pwm->attr[chn_num]->polarity = chn_attr->polarity;

	pwm->create[chn_num] = 1;

	return 0;
}

int SU_PWM_DestroyChn(uint32_t chn_num)
{
	SUPWM *pwm = getpwm();

	if(pwm == NULL) {
		printf("pwm is NULL !\n");
		return -1;
	}

	if((chn_num >= NR_MAX_PWM_NUM) || (chn_num < 0)) {
		printf("chn_num error !\n");
		return -1;
	}

	pwm->create[chn_num] = 0;

	return 0;
}


int SU_PWM_GetChnAttr(uint32_t chn_num, SUPWMChnAttr *chn_attr)
{
	SUPWM *pwm = getpwm();

	if(pwm == NULL) {
		printf("pwm is NULL !\n");
		return -1;
	}

	if((chn_num >= NR_MAX_PWM_NUM) || (chn_num < 0)) {
		printf("chn_num error !\n");
		return -1;
	}

	if(pwm->create[chn_num] == 0) {
		printf("not SU_PWM_CreateChn !\n");
		return -1;
	}

	chn_attr->period   = pwm->attr[chn_num]->period   ;
	chn_attr->duty     = pwm->attr[chn_num]->duty     ;
	chn_attr->polarity = pwm->attr[chn_num]->polarity ;

	return 0;
}

int SU_PWM_SetChnAttr(uint32_t chn_num, SUPWMChnAttr *chn_attr)
{
	SUPWM *pwm = getpwm();

	if(pwm == NULL) {
		printf("pwm is NULL !\n");
		return -1;
	}

	if((chn_num >= NR_MAX_PWM_NUM) || (chn_num < 0)) {
		printf("chn_num error !\n");
		return -1;
	}

	if(pwm->create[chn_num] == 0) {
		printf("not SU_PWM_CreateChn !\n");
		return -1;
	}

	if((chn_attr->period < 200) || (chn_attr->period > 1000000000)) {
		printf("chn_attr->period error !\n");
		return -1;
	}

	if((chn_attr->duty < 0) || (chn_attr->duty > chn_attr->period)) {
		printf("chn_attr->duty error !\n");
		return -1;
	}

	if((chn_attr->polarity < 0) || (chn_attr->polarity > 1)) {
		printf("chn_attr->polarity error !\n");
		return -1;
	}

	pwm->attr[chn_num]->index = chn_num;
	pwm->attr[chn_num]->period = chn_attr->period;
	pwm->attr[chn_num]->duty = chn_attr->duty;
	pwm->attr[chn_num]->polarity = chn_attr->polarity;

	return 0;
}

int SU_PWM_ModifyChnDuty(uint32_t chn_num, int duty)
{
	int ret = 0;
	SUPWM *pwm = getpwm();
	if(pwm == NULL) {
		printf("pwm is NULL !\n");
		return -1;
	}

	if((chn_num >= NR_MAX_PWM_NUM) || (chn_num < 0)) {
		printf("chn_num error !\n");
		return -1;
	}

	if(pwm->create[chn_num] == 0) {
		printf("not SU_PWM_CreateChn !\n");
		return -1;
	}

	if(pwm->enable[chn_num] == 0){
		printf("Please  SU_PWM_Enable firstly!\n");
		return -1;
	}

	if((duty < 0) || (duty > pwm->attr[chn_num]->period)) {
		printf("chn_attr->duty error !\n");
		return -1;
	}
	pwm->attr[chn_num]->duty = duty;

	ret = ioctl(pwm->fd, PWM_CONFIG_DUTY, (unsigned long)pwm->attr[chn_num]);
	if(ret != 0) {
		printf("ioctl : %d error !\n", __LINE__);
		return -1;
	}
	return 0;
}

int SU_PWM_EnableChn(uint32_t chn_num)
{
	int ret;
	SUPWM *pwm = getpwm();

	if(pwm == NULL) {
		printf("pwm is NULL !\n");
		return -1;
	}

	if((chn_num >= NR_MAX_PWM_NUM) || (chn_num < 0)) {
		printf("chn_num error !\n");
		return -1;
	}

	if(pwm->create[chn_num] == 0) {
		printf("not SU_PWM_CreateChn !\n");
		return -1;
	}

	if(pwm->fd < 0) {
		printf("open() error !\n");
		return -1;
	}

	if(pwm->enable[chn_num])
		return 0;

	ret = ioctl(pwm->fd, PWM_CONFIG, (unsigned long)pwm->attr[chn_num]);
	if(ret != 0) {
		printf("ioctl : %d error !\n", __LINE__);
		return -1;
	}

	ret = ioctl(pwm->fd, PWM_ENABLE, chn_num);
	if(ret != 0) {
		printf("ioctl : %d error !\n", __LINE__);
		return -1;
	}

	pwm->enable[chn_num] = 1;

	return 0;
}

int SU_PWM_DisableChn(uint32_t chn_num)
{
	int ret;
	SUPWM *pwm = getpwm();

	if(pwm == NULL) {
		printf("pwm is NULL !\n");
		return -1;
	}

	if((chn_num >= NR_MAX_PWM_NUM) || (chn_num < 0)) {
		printf("chn_num error !\n");
		return -1;
	}

	if(pwm->create[chn_num] == 0) {
		printf("not SU_PWM_CreateChn !\n");
		return -1;
	}

	if(pwm->enable[chn_num] == 0) {
		printf("not SU_PWM_Enable !\n");
		return 0;
	}

	if(pwm->fd < 0) {
		printf("open() error !\n");
		return -1;
	}

	ret = ioctl(pwm->fd, PWM_DISABLE, chn_num);
	if(ret != 0) {
		printf("ioctl : %d error !\n", __LINE__);
		return -1;
	}
	pwm->enable[chn_num] = 0;

	return 0;
}

