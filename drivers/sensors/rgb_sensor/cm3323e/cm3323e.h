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

#ifndef __LINUX_CM3323E_H
#define __LINUX_CM3323E_H

#define CM3323E_I2C_NAME		"cm3323ee"

#define	CM3323E_ADDR			0x20 >> 1
#define CM3323E_SUPPLY_CURRENT  200			//UA

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

struct cm3323e_platform_data {
	int (*power)(int, uint8_t); /* power to the chip */
	uint16_t RGB_slave_address;
};

#endif

static bool g_debugMode;
/*+++BSP David+++*/
#define RGB_TAG "[CM3323E]"
#define ERROR_TAG "[ERR]"
#define RGB_DBG(...)  printk(KERN_INFO RGB_TAG __VA_ARGS__)
#define RGB_DBG_E(...)  printk(KERN_ERR RGB_TAG ERROR_TAG __VA_ARGS__)
#define RGB_DBG_API(...)	if(g_debugMode)\
							printk(KERN_INFO RGB_TAG __VA_ARGS__)
/*---BSP David---*/

#define rearRGB_POLLING_FIRST_RAW	(10)
#define rearRGB_POLLING_TIME		(40)