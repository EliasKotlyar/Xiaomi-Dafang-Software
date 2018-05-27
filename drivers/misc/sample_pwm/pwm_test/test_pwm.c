/*
 * Ingenic SU SDK PWM test.
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 * Author: Tiger <bohu.liang@ingenic.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "pwm.h"

#define PERIOD		10000000
#define DUTY		5000000
#define TIMEOUT		20

int main(int argc, char **argv)
{
	int ret;
	SUPWMChnAttr attr;
	int ch;
	int duty = 0;
	int state = 0;
	int chan = 0;
	int polarity = 0;

	if(argc != 3){
		printf("Please input: ./pwm_test chn polarity\n");
		printf("For example: ./pwm_test 0 1\n");
		return 0;
	}
	chan = atoi(argv[1]);
	polarity = atoi(argv[2]);

	ret = SU_PWM_Init();
	if(ret < 0) {
		printf("SU_PWM_CreateChn error !\n");
		return 0;
	}

	attr.period = PERIOD;
	attr.duty = DUTY;
	attr.polarity = polarity;

	ret = SU_PWM_CreateChn(chan, &attr);
	if(ret < 0) {
		printf("SU_PWM_CreateChn error !\n");
		return 0;
	}


	while (1)
	{
		printf("action：\n");
		printf("1:Enable PWM 2:Disable PWM 3:Modify duty 4:PWM state 5:Exit\n");
		printf("choose:");
		scanf("%d",&ch);
		switch(ch){
			case 1:
				if(state == 0){
					ret = SU_PWM_EnableChn(chan);
					if(ret < 0) {
						printf("SU_PWM_EnableChn error !\n");
						return 0;
					}
					state = 1;
				}
				break;
			case 2:
				if(state){
					SU_PWM_DisableChn(chan);
					state = 0;
				}
				break;
			case 3:
				printf("Please input duty(ns):");
				scanf("%d", &duty);
				ret = SU_PWM_ModifyChnDuty(chan, duty);
				if(ret){
					printf("Failed to modify duty(%d)\n",duty);
					duty = 0;
				}
				attr.duty = duty;
				break;
			case 4:
				printf("PWM state:\n");
				printf("PWM is %s:\n", state ? "RUNNING" :"STOP");
				printf("period: %d; duty: %d; polarity : %d\n", attr.period, attr.duty, attr.polarity);
				break;
			case 5:
				goto done;
			default:
				break;
		}
	}
done:
	if(state)
		SU_PWM_DisableChn(chan);

	SU_PWM_DestroyChn(chan);

	SU_PWM_Exit();

	return 0;
}
