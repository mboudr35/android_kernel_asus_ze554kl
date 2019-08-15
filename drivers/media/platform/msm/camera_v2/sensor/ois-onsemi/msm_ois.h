/* Copyright (c) 2014-2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef MSM_OIS_H
#define MSM_OIS_H

#include <linux/i2c.h>
#include <linux/gpio.h>
#include <soc/qcom/camera2.h>
#include <media/v4l2-subdev.h>
#include <media/msmb_camera.h>
#include "msm_camera_i2c.h"
#include "msm_camera_dt_util.h"
#include "msm_camera_io_util.h"
#include "msm_sd.h"//
#include "msm_cci.h"//

#define DEFINE_MSM_MUTEX(mutexname) \
	static struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)

#define	MSM_OIS_MAX_VREGS (10)

struct msm_ois_ctrl_t;

enum msm_ois_state_t {
	OIS_ENABLE_STATE,
	OIS_OPS_ACTIVE,
	OIS_OPS_INACTIVE,
	OIS_DISABLE_STATE,
};

struct msm_ois_vreg {
	struct camera_vreg_t *cam_vreg;
	void *data[MSM_OIS_MAX_VREGS];
	int num_vreg;
};

/*ASUS_BSP +++ bill_chen "Implement ois"*/
struct msm_ois_board_info {
	char ois_name[MAX_OIS_NAME_SIZE];
	uint16_t i2c_slaveaddr;
	struct msm_camera_power_ctrl_t power_info;
	enum i2c_freq_mode_t i2c_freq_mode;
	//struct msm_ois_opcode opcode;
};
/*ASUS_BSP --- bill_chen "Implement ois"*/

struct msm_ois_ctrl_t {
	struct i2c_driver *i2c_driver;
	struct platform_driver *pdriver;
	struct platform_device *pdev;
	struct msm_camera_i2c_client i2c_client;
	enum msm_camera_device_type_t ois_device_type;
	struct msm_sd_subdev msm_sd;
	struct mutex *ois_mutex;
	enum msm_camera_i2c_data_type i2c_data_type;
	struct v4l2_subdev sdev;
	struct v4l2_subdev_ops *ois_v4l2_subdev_ops;
	void *user_data;
	uint16_t i2c_tbl_index;
	enum cci_i2c_master_t cci_master;
	uint32_t subdev_id;
	enum msm_ois_state_t ois_state;
	struct msm_ois_vreg vreg_cfg;
	struct msm_camera_gpio_conf *gconf;
	struct msm_pinctrl_info pinctrl_info;
	uint8_t cam_pinctrl_status;
	/*ASUS_BSP +++ bill_chen "Implement ois"*/
	struct msm_ois_board_info *oboard_info;
	int32_t userspace_probe;
	//enum i2c_freq_mode_t i2c_freq_mode;
	uint32_t sensor_id_reg_addr;
	uint32_t sensor_id;
	/*ASUS_BSP --- bill_chen "Implement ois"*/
	char vendor[7]; //ASUS_BSP Lucien +++: Add OIS SEMCO module
	uint32_t fw_ver;  //ASUS_BSP Lucien +++: Using service to auto update ois fw
};

extern int g_ftm_mode; //ASUS_BSP PJ_Ma +++

int32_t msm_ois_check_update_fw(struct msm_ois_ctrl_t *o_ctrl, uint32_t mode,
	uint32_t current_fw, uint8_t vcm);

#endif
