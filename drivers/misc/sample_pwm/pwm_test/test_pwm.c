/*
 * Ingenic SU SDK PWM test.
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 * Author: Tiger <bohu.liang@ingenic.com>
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "pwm.h"

#define PERIOD		1000000
#define DUTY		500000
#define DUTY_STEP	50000
#define TIMEOUT		20

int main(void)
{
	int ret;
	SUPWMChnAttr attr;
	char ch;

	ret = SU_PWM_Init();
	if(ret < 0) {
		printf("SU_PWM_CreateChn error !\n");
		return 0;
	}

	attr.period = PERIOD;
	attr.duty = DUTY;
	attr.polarity = 0;

	ret = SU_PWM_CreateChn(2, &attr);
	if(ret < 0) {
		printf("SU_PWM_CreateChn error !\n");
		return 0;
	}

	ret = SU_PWM_EnableChn(2);
	if(ret < 0) {
		printf("SU_PWM_EnableChn error !\n");
		return 0;
	}
	while (1)
	{
		ch = getchar();
        if(ch=='x')
            break;
	}

	SU_PWM_DisableChn(2);

	SU_PWM_DestroyChn(2);

	SU_PWM_Exit();

	return 0;
}
