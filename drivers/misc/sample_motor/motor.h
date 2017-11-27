/*
 * Copyright (C) 2015 Ingenic Semiconductor Co.,Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MOTOR_H__
#define __MOTOR_H__
/****************************** MOTOR ************************************/
#define MOTOR0_MIN_GPIO		-1		/**< motor start point */
#define MOTOR0_MAX_GPIO		-1		/**< motor stop point */
#define MOTOR0_GPIO_LEVEL	0		/**< motor irq style */

#define MOTOR0_ST1_GPIO		GPIO_PB(17)	/**< Phase A */
#define MOTOR0_ST2_GPIO		GPIO_PC(18)	/**< Phase B */
#define MOTOR0_ST3_GPIO		GPIO_PC(2)	/**< Phase C */
#define MOTOR0_ST4_GPIO		GPIO_PC(3)	/**< Phase D */

#define MOTOR0_MAX_SPEED	1000		/**< unit : ns */
#define MOTOR0_MIN_SPEED	10

#define MOTOR1_MIN_GPIO		-1
#define MOTOR1_MAX_GPIO		-1
#define MOTOR1_GPIO_LEVEL	0

#define MOTOR1_ST1_GPIO		GPIO_PC(4)
#define MOTOR1_ST2_GPIO		GPIO_PC(5)
#define MOTOR1_ST3_GPIO		GPIO_PC(6)
#define MOTOR1_ST4_GPIO		GPIO_PC(7)

#define MOTOR1_MAX_SPEED	1000
#define MOTOR1_MIN_SPEED	10

/****************************** MOTOR END ************************************/
/* rrun status */
#define MOTOR_MOVE_STOP		0x0
#define MOTOR_MOVE_RUN		0x1

/* directional_attr */
#define MOTOR_DIRECTIONAL_UP	0x0
#define MOTOR_DIRECTIONAL_DOWN	0x1
#define MOTOR_DIRECTIONAL_LEFT	0x2
#define MOTOR_DIRECTIONAL_RIGHT	0x3

/* ioctl cmd */
#define MOTOR_STOP		0x1
#define MOTOR_RESET		0x2
#define MOTOR_MOVE		0x3
#define MOTOR_GET_STATUS	0x4
#define MOTOR_SPEED		0x5

/* init speed */
#define MOTOR_INIT_SPEED	1000	/* unit :ns */

struct motor_move_st {
	int motor_directional;
	int motor_move_steps;
	int motor_move_speed;
};

struct motor_status_st {
	int directional_attr;
	int total_steps;
	int current_steps;
	int min_speed;
	int cur_speed;
	int max_speed;
	int move_is_min;
	int move_is_max;
};

struct motor_platform_data {
	const char *name;
	int directional_attr;

	unsigned int motor_min_gpio;
	unsigned int motor_max_gpio;
	int motor_gpio_level;

	unsigned int motor_st1_gpio;
	unsigned int motor_st2_gpio;
	unsigned int motor_st3_gpio;
	unsigned int motor_st4_gpio;

	int max_speed;
	int min_speed;
	int cur_speed;
	int step_angle;
};

struct motor_info {
	struct semaphore semaphore;
	struct platform_device *pdev;
	const struct mfd_cell *cell;
	struct device	 *dev;
	struct miscdevice mdev;
	struct regulator *power;
	struct motor_platform_data *pdata[2];
	struct mutex lock;
	struct task_struct *motor_thread;
	struct completion time_completion;
	struct jz_tcu_chn *tcu;
	wait_queue_head_t motor_wq;
	int status;
	int total_steps[2];
	int current_steps[2];
	int run_step;
	int run_step_irq;
	int speed;
	int move_is_min;
	int move_is_max;
	int direction;
	int id;
	int cur_steps[4];
	int set_steps[4];
};

#define DEF_MOTOR(NO) \
	 struct motor_platform_data jz_motor##NO##_pdata = { \
	    .name = "jz-tcu", \
	    .motor_min_gpio = MOTOR##NO##_MIN_GPIO, \
	    .motor_max_gpio = MOTOR##NO##_MAX_GPIO, \
	    .motor_gpio_level = MOTOR##NO##_GPIO_LEVEL, \
	    .motor_st1_gpio = MOTOR##NO##_ST1_GPIO, \
	    .motor_st2_gpio = MOTOR##NO##_ST2_GPIO, \
	    .motor_st3_gpio = MOTOR##NO##_ST3_GPIO, \
	    .motor_st4_gpio = MOTOR##NO##_ST4_GPIO, \
	    .max_speed = MOTOR##NO##_MAX_SPEED, \
	    .min_speed = MOTOR##NO##_MIN_SPEED, \
	};

#endif // __MOTOR_H__
