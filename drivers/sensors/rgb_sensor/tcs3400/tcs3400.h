/* include/linux/cm3323e.h
 *
 * Copyright (C) 2012 Capella Microsystems Inc.
 * Author: Frank Hsieh <pengyueh@gmail.com>  
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __LINUX_TCS3400_H
#define __LINUX_TCS3400_H

#define TCS3400_I2C_NAME		"tcs3400"

#define	TCS3400_ADDR			0x20 >> 1

/* cm3323e command code */
#define	CM3323E_CONF			0x00
#define	CM3323E_RESERVE	0x03
#define CM3323E_R_DATA		0x08
#define CM3323E_G_DATA		0x09
#define CM3323E_B_DATA		0x0A
#define CM3323E_W_DATA		0x0B

/* cm3323e CONF command code */
#define CM3323E_CONF_SD			1
#define CM3323E_CONF_AF			(1 << 1)
#define CM3323E_CONF_TRIG		(1 << 2)
#define CM3323E_CONF_IT_40MS		(0 << 4)
#define CM3323E_CONF_IT_80MS		(1 << 4)
#define CM3323E_CONF_IT_160MS	(2 << 4)
#define CM3323E_CONF_IT_320MS	(3 << 4)
#define CM3323E_CONF_IT_640MS	(4 << 4)
#define CM3323E_CONF_IT_1280MS	(5 << 4)
#define CM3323E_CONF_DEFAULT		0

#define LS_PWR_ON					(1 << 0)

struct tcs3400_platform_data {
	int (*power)(int, uint8_t); /* power to the chip */
	uint16_t RGB_slave_address;
};

#endif

static bool g_debugMode;
static bool rgb_debugMode = false;
/*+++BSP David+++*/
#define RGB_TAG "[TCS3400]"
#define ERROR_TAG "[ERR]"
#define DEBUG_TAG "[DEBUG]"
#define RGB_DBG(...)  printk(KERN_INFO RGB_TAG __VA_ARGS__)
#define RGB_DBG_E(...)  printk(KERN_ERR RGB_TAG ERROR_TAG __VA_ARGS__)
#define RGB_DBG_D(...)  if(rgb_debugMode)\
							printk(KERN_INFO RGB_TAG DEBUG_TAG __VA_ARGS__)
#define RGB_DBG_API(...)	if(g_debugMode)\
							printk(KERN_INFO RGB_TAG __VA_ARGS__)
/*---BSP David---*/

/* +++BSP Vincent */

#define RGB_SENSOR_COMPATIBLE_NAME "ams,tcs3400"
#define RGB_SENSOR_GPIO_IRQ "qcom,alsps-gpio"
#define RGB_SENSOR_INT_NAME "tcs3400"


#define TCS3400_CONTROL				0x80
#define TCS3400_ALS_TIME			0x81
#define TCS3400_PERSISTENCE			0x8C
#define TCS3400_PRX_PULSE_COUNT		0x8E
#define TCS3400_GAIN				0x8F

#define TCS3400_STATUS				0x93

#define TCS3400_CLR_CHANLO			0x94
#define TCS3400_CLR_CHANHI			0x95
#define TCS3400_RED_CHANLO			0x96
#define TCS3400_RED_CHANHI			0x97
#define TCS3400_GRN_CHANLO			0x98
#define TCS3400_GRN_CHANHI			0x99
#define TCS3400_BLU_CHANLO			0x9A
#define TCS3400_BLU_CHANHI			0x9B

#define TCS3400_PRX_OFFSET			0x9E

#define ALS_PERSIST(p) (((p) & 0xf) << 3)

enum tcs3400_ctrl_reg {
	AGAIN_1        = (0 << 0),
	AGAIN_4        = (1 << 0),
	AGAIN_16       = (2 << 0),
	AGAIN_64       = (3 << 0),
};

enum tcs3400_en_reg {
	TCS3400_EN_PWR_ON   = (1 << 0),
	TCS3400_EN_ALS      = (1 << 1),
	TCS3400_EN_PRX      = (1 << 2),
	TCS3400_EN_WAIT     = (1 << 3),
	TCS3400_EN_ALS_IRQ  = (1 << 4),
	TCS3400_EN_PRX_IRQ  = (1 << 5),
	TCS3400_EN_IRQ_PWRDN = (1 << 6),
	TCS3400_EN_BEAM     = (1 << 7),
};

/* ---BSP Vincent */