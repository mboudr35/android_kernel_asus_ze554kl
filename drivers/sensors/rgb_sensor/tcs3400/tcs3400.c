/* drivers/input/misc/tcs3400.c - tcs3400 optical sensors driver
 *
 * Copyright (C) 2015 Vishay Capella Microsystems Inc.
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

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/setup.h>
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/ioctl.h>
#include <media/msm_cam_sensor.h>
//#include "../../media/platform/msm/camera_v2/sensor/io/msm_camera_dt_util.h"
//#include "../../media/platform/msm/camera_v2/common/msm_camera_io_util.h"
#include "tcs3400.h"
#include "lightsensor.h"

#ifdef CONFIG_I2C_STRESS_TEST
#include <linux/i2c_testcase.h>
#define I2C_TEST_TCS3400_FAIL (-1)
#endif

#define POWER_ALWAYS_ON

#define ASUS_RGB_SENSOR_DATA_SIZE	5
#define ASUS_RGB_SENSOR_NAME_SIZE	32
#define ASUS_RGB_SENSOR_IOC_MAGIC                      ('C')	///< RGB sensor ioctl magic number 
#define ASUS_RGB_SENSOR_IOCTL_DATA_READ           _IOR(ASUS_RGB_SENSOR_IOC_MAGIC, 1, int[ASUS_RGB_SENSOR_DATA_SIZE])	///< RGB sensor ioctl command - Read data RGBW
#define ASUS_RGB_SENSOR_IOCTL_IT_SET           _IOW(ASUS_RGB_SENSOR_IOC_MAGIC, 2, int)	///< RGB sensor ioctl command - Set integration time
#define ASUS_RGB_SENSOR_IOCTL_DEBUG_MODE           _IOW(ASUS_RGB_SENSOR_IOC_MAGIC, 3, int)	///< RGB sensor ioctl command - Get debug mode
#define ASUS_RGB_SENSOR_IOCTL_MODULE_NAME           _IOR(ASUS_RGB_SENSOR_IOC_MAGIC, 4, char[ASUS_RGB_SENSOR_NAME_SIZE])	///< RGB sensor ioctl command - Get module name

#define D(x...) pr_info(x)

#define I2C_RETRY_COUNT 10
#define IT_TIME_LOG_SAMPLE_RATE 50

#define LS_POLLING_DELAY 600

#define REL_RED		REL_X
#define REL_GREEN	REL_Y
#define REL_BLUE	REL_Z
#define REL_WHITE	REL_MISC

#define CFG_IT_MASK            0x0070
#define CFG_SD_MASK            0x0001

#define RGB_INDEX_R 0
#define RGB_INDEX_G 1
#define RGB_INDEX_B 2
#define RGB_INDEX_W 3

#define RGB_SENSOR_FILE  		"/factory/rgbSensor_data"

/*====================
 *|| I2C mutex lock ||
 *====================*/

//+++Vincent
//extern void lock_i2c_bus6(void);
//extern void unlock_i2c_bus6(void);
#if 0
static struct mutex	g_i2c_lock;

void lock_i2c_bus7(void) {
	mutex_lock(&g_i2c_lock);
}
EXPORT_SYMBOL(lock_i2c_bus7);

void unlock_i2c_bus7(void) {
	mutex_unlock(&g_i2c_lock);
}
EXPORT_SYMBOL(unlock_i2c_bus7);
#endif
//---Vincent


static struct mutex als_enable_mutex;
struct msm_rgb_sensor_vreg {
	struct camera_vreg_t *cam_vreg;
	void *data[CAM_VREG_MAX];
	int num_vreg;
};
struct tcs3400_info {
	struct class *tcs3400_class;
	struct device *ls_dev;
	struct input_dev *ls_input_dev;

	struct i2c_client *i2c_client;
	struct workqueue_struct *lp_wq;
	struct msm_rgb_sensor_vreg vreg_cfg;

	int als_enable;
	int (*power)(int, uint8_t); /* power to the chip */

	int lightsensor_opened;
	int polling_delay;
	u8 it_time;
	u8 gain;
	bool using_calib;
};
struct tcs3400_status {
	bool need_wait;
	u32 start_time_jiffies;
	u32 end_time_jiffies;
	u16 delay_time_ms;
	u16 delay_time_jiffies;
};
static struct tcs3400_info *cm_lp_info;
static struct tcs3400_status g_rgb_status = {false, 0, 0};

enum RGB_data {
	RGB_DATA_R = 0,
	RGB_DATA_G,
	RGB_DATA_B,
	RGB_DATA_W,
};
enum RGB_it {
	RGB_IT_40MS = 0,
	RGB_IT_80MS,
	RGB_IT_160MS,
	RGB_IT_320MS,
	RGB_IT_640MS,
	RGB_IT_1280MS,
};

#if 0
static int I2C_RxData(uint16_t slaveAddr, uint8_t cmd, uint8_t *rxData, int length)
{
	uint8_t loop_i;
	uint8_t subaddr[1];

	struct i2c_msg msg[] = {
		{
		 .addr = slaveAddr,
		 .flags = 0,
		 .len = 1,
		 .buf = subaddr,
		 },
		{
		 .addr = slaveAddr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxData,
		 },
	};
	subaddr[0] = cmd;

	for (loop_i = 0; loop_i < I2C_RETRY_COUNT; loop_i++) {
		if (i2c_transfer(cm_lp_info->i2c_client->adapter, msg, 2) > 0)
			break;

		msleep(10);
	}
	if (loop_i >= I2C_RETRY_COUNT) {
		RGB_DBG_E("%s retry over %d\n", __func__, I2C_RETRY_COUNT);
		return -EIO;
	}

	return 0; 
}

static int I2C_TxData(uint16_t slaveAddr, uint8_t *txData, int length)
{
	uint8_t loop_i;

	struct i2c_msg msg[] = {
		{
		 .addr = slaveAddr,
		 .flags = 0,
		 .len = length,
		 .buf = txData,
		 },
	};

	for (loop_i = 0; loop_i < I2C_RETRY_COUNT; loop_i++) {
		if (i2c_transfer(cm_lp_info->i2c_client->adapter, msg, 1) > 0)
			break;

		msleep(10);
	}

	if (loop_i >= I2C_RETRY_COUNT) {
		RGB_DBG_E("%s retry over %d\n", __func__, I2C_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

static int _tcs3400_I2C_Read_Word(uint16_t slaveAddr, uint8_t cmd, uint16_t *pdata)
{
	uint8_t buffer[2];
	int ret = 0;

	if (pdata == NULL)
		return -EFAULT;

	ret = I2C_RxData(slaveAddr, cmd, buffer, 2);
	if (ret < 0) {
		RGB_DBG_E("%s: I2C_RxData fail [0x%x, 0x%x]\n", __func__, slaveAddr, cmd);
		return ret;
	}

	*pdata = (buffer[1] << 8) | buffer[0];
#if 0
	/* Debug use */
	RGB_DBG("%s: I2C_RxData[0x%x, 0x%x] = 0x%x\n", __func__, slaveAddr, cmd, *pdata);
#endif
	return ret;
}

static int _tcs3400_I2C_Write_Word(uint16_t SlaveAddress, uint8_t cmd, uint16_t data)
{
	char buffer[3];
	int ret = 0;
#if 0
	/* Debug use */
	RGB_DBG("%s: _tcs3400_I2C_Write_Word[0x%x, 0x%x, 0x%x]\n", __func__, SlaveAddress, cmd, data);
#endif
	buffer[0] = cmd;
	buffer[1] = (uint8_t)(data & 0xff);
	buffer[2] = (uint8_t)((data & 0xff00) >> 8);

	ret = I2C_TxData(SlaveAddress, buffer, 3);
	if (ret < 0) {
		RGB_DBG_E("%s: I2C_TxData fail\n", __func__);
		return -EIO;
	}

	return ret;
}

static int _tcs3400_I2C_Mask_Write_Word(uint16_t SlaveAddress, uint8_t cmd, uint16_t mask, uint16_t data)
{
	s32 rc;
 	uint16_t adc_data;

	rc = _tcs3400_I2C_Read_Word(SlaveAddress, cmd, &adc_data);
	if (rc) {
		RGB_DBG_E("%s: Failed to read reg 0x%02x, rc=%d\n", __func__, cmd, rc);
		goto out;
	}
	adc_data &= ~mask;
	adc_data |= data & mask;
	rc = _tcs3400_I2C_Write_Word(SlaveAddress, cmd, adc_data);
	if (rc) {
		RGB_DBG_E("%s: Failed to write reg 0x%02x, rc=%d\n", __func__, cmd, rc);
	}
out:
	return rc;
}


#endif

static int tcs3400_i2c_read(u8 reg, u8 *val)
{
	int ret;

	s32 read;
	struct i2c_client *client = cm_lp_info->i2c_client;	

	ret = i2c_smbus_write_byte(client, reg);
	if (ret < 0) {
		mdelay(3);
		ret = i2c_smbus_write_byte(client, reg);
		if (ret < 0) {
			RGB_DBG_E("%s: failed 2x to write register %x\n",
				__func__, reg);
		return ret;
		}
	}

	read = i2c_smbus_read_byte(client);
	if (read < 0) {
		mdelay(3);
		read = i2c_smbus_read_byte(client);
		if (read < 0) {
			RGB_DBG_E("%s: failed read from register %x\n",
				__func__, reg);
		}
		return ret;
	}	

	*val = (u8)read;

	RGB_DBG_D("%s: read 0x%x = 0x%x\n", __func__, reg, *val);
	
	return 0;
}

static int tcs3400_i2c_write(u8 reg, u8 val)
{
	int ret;
	struct i2c_client *client = cm_lp_info->i2c_client;

	RGB_DBG_D("%s: set 0x%x = 0x%x\n", __func__, reg, val);

	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		mdelay(3);
		ret = i2c_smbus_write_byte_data(client, reg, val);
		if (ret < 0) {
			RGB_DBG_E("%s: failed to write register %x err= %d\n",
				__func__, reg, ret);
		}
	}
	return ret;
}

static int tcs3400_flush_regs(void)
{

	int rc;
	rc = tcs3400_i2c_write(TCS3400_ALS_TIME, cm_lp_info->it_time);
	if (rc) {
		return rc;
	}

	rc = tcs3400_i2c_write(TCS3400_PERSISTENCE, ALS_PERSIST(0));
	if (rc) {
		return rc;
	}	

	/* TCS3400_PRX_PULSE_COUNT 0x8E is not used for TCS3400 */
	rc = tcs3400_i2c_write(TCS3400_PRX_PULSE_COUNT, 0);
	if (rc) {
		return rc;
	}

	rc = tcs3400_i2c_write(TCS3400_GAIN, cm_lp_info->gain);
	if (rc) {
		return rc;
	}

	/* dont know what it is */
	rc = tcs3400_i2c_write(TCS3400_PRX_OFFSET, 0);
	if (rc) {
		return rc;
	}

	return rc;
}


//+++[Vincent][Remove]-porting
#if 0
static int32_t als_power(int config)
{
	int rc = 0, i, cnt;
	struct msm_rgb_sensor_vreg *vreg_cfg;
	if (!cm_lp_info) {
		RGB_DBG_E("%s: null pointer, probe might not finished!\n", __func__);
		return -1;
	}
	vreg_cfg = &cm_lp_info->vreg_cfg;
	cnt = vreg_cfg->num_vreg;
	if (!cnt)
		return 0;

	if (cnt >= CAM_VREG_MAX) {
		pr_err("%s failed %d cnt %d\n", __func__, __LINE__, cnt);
		return -EINVAL;
	}

	for (i = 0; i < cnt; i++) {
		rc = msm_camera_config_single_vreg(&(cm_lp_info->i2c_client->dev),
			&vreg_cfg->cam_vreg[i],
			(struct regulator **)&vreg_cfg->data[i],
			config);
	}
	return rc;
}

int rgbSensor_getUserSpaceData(char* buf, int len)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos_lsts = 0;
	int readlen = 0;	
	
	fp = filp_open(RGB_SENSOR_FILE, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR_OR_NULL(fp)) {
		RGB_DBG_E("%s: open %s fail\n", __func__, RGB_SENSOR_FILE);
		return -ENOENT;	/*No such file or directory*/
	}

	/*For purpose that can use read/write system call*/
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (fp->f_op != NULL && fp->f_op->read != NULL) {
		pos_lsts = 0;
		readlen = fp->f_op->read(fp, buf, len, &pos_lsts);
		buf[readlen] = '\0';		
	} else {
		RGB_DBG_E("%s: f_op=NULL or op->read=NULL\n", __func__);
		return -ENXIO;	/*No such device or address*/
	}
	set_fs(old_fs);
	filp_close(fp, NULL);
	return 0;
}
#endif
//---[Vincent][Remove]-porting

static void rgbSensor_setDelay(void)
{
	g_rgb_status.need_wait = true;
	g_rgb_status.start_time_jiffies = jiffies;
#if 0	
	if (cm_lp_info->it_time >= 0 && cm_lp_info->it_time <= 5) {
		g_rgb_status.delay_time_ms = (40 << cm_lp_info->it_time) * 5 / 4;
		RGB_DBG("%s: set delay time to %d ms\n", __func__, g_rgb_status.delay_time_ms);
	} else{
		g_rgb_status.delay_time_ms = 200;
		RGB_DBG_E("%s: wrong IT time - %d, set delay time to 200ms \n", __func__, cm_lp_info->it_time);
	}
#endif
	g_rgb_status.delay_time_ms = (256 - cm_lp_info->it_time) * 278 / 100 * 5 / 4;
	RGB_DBG("%s: set delay time to %d ms\n", __func__, g_rgb_status.delay_time_ms);	
	g_rgb_status.delay_time_jiffies = g_rgb_status.delay_time_ms * HZ / 1000;
	g_rgb_status.end_time_jiffies = g_rgb_status.start_time_jiffies + g_rgb_status.delay_time_jiffies;
}

#if 0
void rgbSensor_workAround(void)
{
	if (cm_lp_info) {
		_tcs3400_I2C_Write_Word(TCS3400_ADDR, TCS3400_RESERVE, 0);
		RGB_DBG("%s: done\n", __func__);
	} else{
		RGB_DBG_E("%s: rgb sensor isn't registered\n", __func__);
	}
}
#endif

static int rgbSensor_doEnable(bool enabled)
{
	int ret = 0;
	if (enabled) {
		cm_lp_info->als_enable = 1;
#ifndef POWER_ALWAYS_ON		
		//+++[Vincent][temp]
		//lock_i2c_bus7();
		//---Vincent
//+++[Vincent][Remove]-porting
		//als_power(1);
//---[Vincent][Remove]-porting
		//rgbSensor_workAround();
		//+++Vincent
		//unlock_i2c_bus7();
		//---Vincent
#endif
		msleep(5);
		tcs3400_flush_regs();
		ret = tcs3400_i2c_write(TCS3400_CONTROL, TCS3400_EN_PWR_ON | TCS3400_EN_ALS | TCS3400_EN_ALS_IRQ);
		rgbSensor_setDelay();
	} else{
		cm_lp_info->als_enable = 0;
		ret = tcs3400_i2c_write(TCS3400_CONTROL, 0);
#ifndef POWER_ALWAYS_ON
//+++[Vincent][Remove]-porting	
		//als_power(0);
//---[Vincent][Remove]-porting
#endif
	}
	return ret;
}
static int rgbSensor_setEnable(bool enabled)
{
	int ret = 0;
	static int l_count = 0;
	bool do_enabled = false;

	if (!cm_lp_info) {
		RGB_DBG_E("%s: null pointer, probe might not finished!\n", __func__);
		return -1;
	}
	RGB_DBG("%s: count = %d, enabled = %d\n", __func__, l_count, enabled ? 1 : 0);
	mutex_lock(&als_enable_mutex);
	if ((enabled && l_count == 0) || (!enabled && l_count == 1)) {
		ret = rgbSensor_doEnable(enabled);
		do_enabled = enabled;
	}
	if (enabled) {
		l_count++;
	} else{
		l_count--;
	}
	mutex_unlock(&als_enable_mutex);

	return ret;
}
#if 0
static bool rgbSensor_getEnable(void)
{
	bool result;
	if (!cm_lp_info) {
		result = false;
	} else{
		mutex_lock(&als_enable_mutex);
		if (cm_lp_info->als_enable == 1) {
			result = true;
		} else{
			result = false;
		}
		mutex_unlock(&als_enable_mutex);
	}
	return result;
}
#endif
static void rgbSensor_doDelay(u32 delay_time_jiffies)
{
	u32 delay_time_ms = delay_time_jiffies * 1000 / HZ;
	RGB_DBG("%s: delay time = %u(ms)\n", __func__, delay_time_ms);
	if (delay_time_ms < 0 || delay_time_ms > g_rgb_status.delay_time_ms) {
		RGB_DBG_E("%s: param wrong, delay default time = %u(ms)\n", __func__, g_rgb_status.delay_time_ms);
		msleep(g_rgb_status.delay_time_ms);
	} else{
		msleep(delay_time_ms);
	}
}
static void rgbSensor_checkStatus(void)
{
	u32 current_tme_l = jiffies;
	if (g_rgb_status.need_wait) {
		/*handling jiffies overflow*/
		RGB_DBG("%s: start at %u, end at %u, current at %u, delay = %u(jiffies)\n", __func__,
			g_rgb_status.start_time_jiffies, g_rgb_status.end_time_jiffies, current_tme_l, g_rgb_status.delay_time_jiffies);
		/*normal case*/
		if (g_rgb_status.end_time_jiffies > g_rgb_status.start_time_jiffies) {
			/*between start and end - need delay*/
			if (current_tme_l >= g_rgb_status.start_time_jiffies && current_tme_l < g_rgb_status.end_time_jiffies) {
				rgbSensor_doDelay(g_rgb_status.end_time_jiffies - current_tme_l);
			} else{
			}
		/*overflow case*/
		} else{
			/*after end - don't need to delay*/
			if (current_tme_l < g_rgb_status.start_time_jiffies && current_tme_l >= g_rgb_status.end_time_jiffies) {
			} else{
				if (current_tme_l >= g_rgb_status.start_time_jiffies) {
					rgbSensor_doDelay(g_rgb_status.delay_time_jiffies - (current_tme_l - g_rgb_status.start_time_jiffies));
				} else{
					rgbSensor_doDelay(g_rgb_status.end_time_jiffies - current_tme_l);
				}
			}
		}
		g_rgb_status.need_wait = false;
	}
}
static int get_rgb_data(int rgb_data, u16 *pdata, bool l_needDelay)
{
	int ret = 0;
	u8 cmd1, cmd2;
	u8 raw_data[2];
	if (cm_lp_info == NULL || cm_lp_info->als_enable == 0) {
		RGB_DBG_E("%s: als not enabled yet!\n", __func__);
		ret = -1;
		goto end;
	}
	switch (rgb_data) {
	case RGB_DATA_R:
		RGB_DBG_D("Get RED raw data\n");
		cmd1 = TCS3400_RED_CHANLO;
		cmd2 = TCS3400_RED_CHANHI;
		break;
	case RGB_DATA_G:
		RGB_DBG_D("Get GREEN raw data\n");
		cmd1 = TCS3400_GRN_CHANLO;
		cmd2 = TCS3400_GRN_CHANHI;
		break;
	case RGB_DATA_B:
		RGB_DBG_D("Get BLUE raw data\n");
		cmd1 = TCS3400_BLU_CHANLO;
		cmd2 = TCS3400_BLU_CHANHI;
		break;
	case RGB_DATA_W:
		RGB_DBG_D("Get CLEAR raw data\n");
		cmd1 = TCS3400_CLR_CHANLO;
		cmd2 = TCS3400_CLR_CHANHI;
		break;
	default:
		RGB_DBG_E("%s: unknown cmd(%d)\n", __func__, rgb_data);
		ret = -2;
		goto end;
	}
	if (l_needDelay) {
		rgbSensor_checkStatus();
	}

	ret = tcs3400_i2c_read(cmd1, &raw_data[0]);
	if (ret < 0) {
		RGB_DBG_E("%s: tcs3400_i2c_read at %x fail\n", __func__, cmd1);
		goto end;
	}

	RGB_DBG_D("Low byte: 0x%x\n", raw_data[0]);

	ret = tcs3400_i2c_read(cmd2, &raw_data[1]);	
	if (ret < 0) {
		RGB_DBG_E("%s: tcs3400_i2c_read at %x fail\n", __func__, cmd2);
		goto end;
	}
	RGB_DBG_D("High byte: 0x%x\n", raw_data[1]);

	*pdata = le16_to_cpup((const __le16 *)&raw_data[0]);

	RGB_DBG("raw_data: 0x%x, %d\n", *pdata, *pdata);
	
end:
	return ret;
}
static int rgbSensor_setup(struct tcs3400_info *lpi)
{
	int ret = 0;
 	uint16_t adc_data;

	lpi->it_time = 0x6B;
	lpi->gain = AGAIN_16;

	// Enable tcs3400
	ret = rgbSensor_setEnable(true);
	if(ret < 0) {
		return ret;
	}

	// Get initial RED light data
	ret = get_rgb_data(RGB_DATA_R, &adc_data, false);
	if (ret < 0) {
		return -EIO;
	}
	
	// Get initial GREEN light data
	ret = get_rgb_data(RGB_DATA_G, &adc_data, false);
	if (ret < 0) {
		return -EIO;
	}

	// Get initial BLUE light data
	ret = get_rgb_data(RGB_DATA_B, &adc_data, false);
	if (ret < 0) {
		return -EIO;
	}

	// Get initial WHITE light data
	ret = get_rgb_data(RGB_DATA_W, &adc_data, false);
	if (ret < 0) {
		return -EIO;
	}

	ret = rgbSensor_setEnable(false);
	
	return ret;
}
/*+++BSP David ASUS Interface+++*/

static bool rgbSensor_checkI2C(void)
{
	int ret = -1;
 	uint8_t adc_status;
	uint16_t adc_data;
	// Get initial RED light data
	rgbSensor_setEnable(true);
	
	ret = tcs3400_i2c_read(TCS3400_STATUS, &adc_status);
	if (ret < 0) {
		goto end;
	}
	
	ret = get_rgb_data(RGB_DATA_R, &adc_data, true);
	if (ret < 0) {
		goto end;
	}

end:

	rgbSensor_setEnable(false);

	if (ret < 0) {
		RGB_DBG_E("%s: fail\n", __func__);
		return false;
	} else{
		RGB_DBG("%s: %u\n", __func__, adc_data);
		return true;
	}
}
static int rgbSensor_itSet_ms(int input_ms)
{
	static uint16_t l_count = 0;
	u8 it_time;
	int ret = 0;
	int cycle_count;
#if 0	
	if (input_ms < 80) {
		it_time = 0;
	} else if (input_ms < 160) {
		it_time = 1;
	} else if (input_ms < 320) {
		it_time = 2;
	} else if (input_ms < 640) {
		it_time = 3;
	} else if (input_ms < 1280) {
		it_time = 4;
	} else{
		it_time = 5;
	}
#endif
	cycle_count = input_ms * 100 / 278;
	if(cycle_count < 1)
		cycle_count = 1;

	if(cycle_count > 256)
		cycle_count = 256;

	it_time = 256 - cycle_count;
	

	if (l_count % IT_TIME_LOG_SAMPLE_RATE == 0) {
		RGB_DBG("%s: it time = %d, cycles = %d, it set = %d, log rate = %d\n", __func__, input_ms, cycle_count, it_time, IT_TIME_LOG_SAMPLE_RATE);
		l_count = 0;
	}
	l_count++;
	cm_lp_info->it_time = it_time;
	if (cm_lp_info->als_enable == 1) {
		ret = tcs3400_i2c_write(TCS3400_ALS_TIME, it_time);
	} else{
		ret = -1;
		RGB_DBG("%s: write config - it time = %d, it set = %d\n", __func__, input_ms, it_time);
	}
	return ret;
}

static int rgbSensor_gainSet(int val)
{
	u8 gain_value;
	int ret;

	switch(val){
	case 1:
		gain_value = AGAIN_1;
		break;
	case 4:
		gain_value = AGAIN_4;
		break;
	case 16:
		gain_value = AGAIN_16;
		break;
	case 64:
		gain_value = AGAIN_64;
		break;
	default:
		RGB_DBG_E("%s: invalid parameter: %d\n", __func__, val);
		return -1;
	}

	cm_lp_info->gain = gain_value;

	ret = tcs3400_i2c_write(TCS3400_GAIN, cm_lp_info->gain);
	return ret;
}

static int tcs3400_als_showrgbc_proc_read(struct seq_file *buf, void *v)
{
	u16 adc_data[4];
	u8 adc_status;
	int ret;

	rgbSensor_setEnable(true);

	/* +++Vincent- workaround for reading wrong value */
	ret = tcs3400_i2c_read(TCS3400_STATUS, &adc_status);
	if (ret < 0) {
		return -EIO;
	}
	
	// Get initial RED light data
	ret = get_rgb_data(RGB_DATA_R, &adc_data[0], true);
	if (ret < 0) {
		return -EIO;
	}
	ret = get_rgb_data(RGB_DATA_G, &adc_data[1], true);
	if (ret < 0) {
		return -EIO;
	}
	ret = get_rgb_data(RGB_DATA_B, &adc_data[2], true);
	if (ret < 0) {
		return -EIO;
	}
	ret = get_rgb_data(RGB_DATA_W, &adc_data[3], true);
	if (ret < 0) {
		return -EIO;
	}

	rgbSensor_setEnable(false);

	RGB_DBG("RGBW raw data: %d;%d;%d;%d\n", adc_data[0], adc_data[1], adc_data[2], adc_data[3]);	
	seq_printf(buf, "%d;%d;%d;%d\n", adc_data[0], adc_data[1], adc_data[2], adc_data[3]);

	return 0;
	
}

static int tcs3400_als_showrgbc_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, tcs3400_als_showrgbc_proc_read, NULL);
}

static void create_rgbSensor_showrgbc_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  tcs3400_als_showrgbc_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_showrgbc", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}

static ssize_t tcs3400_als_gain_proc_write(struct file *filp, const char __user *buff,
		size_t len, loff_t *data)
{
	unsigned long val;
	int ret;
	char messages[256] = {0};
	if (len > 256) {
		len = 256;
	}
	if (copy_from_user(messages, buff, len)) {
		return -EFAULT;
	}

	RGB_DBG("%s: The gain you set is %s\n", __func__, messages);

	if ((ret = kstrtoul(messages, 10, &val)) != 0){
		RGB_DBG_E("%s: error! kstrtol fail, val = %ld bbb, error code = %d\n", __func__, val, ret);
		return ret;
	}

	rgbSensor_setEnable(true);
	RGB_DBG("%s: %ld\n", __func__, val);
	ret = rgbSensor_gainSet((int)val);
	rgbSensor_setEnable(false);

	if(ret < 0)
		return -EFAULT;	

	return len;
	
}

static int tcs3400_als_gain_proc_read(struct seq_file *buf, void *v)
{
	u8 val;
	int gain;

	rgbSensor_setEnable(true);
	tcs3400_i2c_read(TCS3400_GAIN, &val);
	rgbSensor_setEnable(false);

	switch (val) {
	case AGAIN_1:
		gain = 1;
		break;
	case AGAIN_4:
		gain = 4;
		break;
	case AGAIN_16:
		gain = 16;
		break;
	case AGAIN_64:
		gain = 64;
		break;	
	default:
		gain = (int)val;
	}

	seq_printf(buf, "%d\n", gain);

	return 0;

}


static int tcs3400_als_gain_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, tcs3400_als_gain_proc_read, NULL);
}

static void create_rgbSensor_gain_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  tcs3400_als_gain_proc_open,
		.read = seq_read,
		.write = tcs3400_als_gain_proc_write,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_gain", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}


static ssize_t tcs3400_als_enable_proc_write(struct file *filp, const char __user *buff,
		size_t len, loff_t *data)
{
	int ret;
	bool enable;
	int temp;
	char messages[256] = {0};
	if (len > 256) {
		len = 256;
	}
	if (copy_from_user(messages, buff, len)) {
		return -EFAULT;
	}

	sscanf(messages, "%d", &temp);
	enable = !!temp;


	RGB_DBG("%s: %s RGB sensor\n", __func__, enable ? "Enable" : "Disable");

	ret = rgbSensor_setEnable(enable);

	if(ret < 0)
		return -EFAULT;	

	return len;
	
}

static int tcs3400_als_enable_proc_read(struct seq_file *buf, void *v)
{
	seq_printf(buf, "%d\n", cm_lp_info->als_enable);
	return 0;
}


static int tcs3400_als_enable_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, tcs3400_als_enable_proc_read, NULL);
}

static void create_rgbSensor_enable_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  tcs3400_als_enable_proc_open,
		.read = seq_read,
		.write = tcs3400_als_enable_proc_write,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_enable", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}



static int tcs3400_als_register_proc_read(struct seq_file *buf, void *v)
{
	return 0;
}
static int tcs3400_als_register_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, tcs3400_als_register_proc_read, NULL);
}

static ssize_t tcs3400_als_register_proc_write(struct file *filp, const char __user *buff,
		size_t len, loff_t *data)
{
	int ret;
	int i2c_reg_addr, i2c_reg_val;
	char messages[256] = {0};
	if (len > 256) {
		len = 256;
	}
	if (copy_from_user(messages, buff, len)) {
		return -EFAULT;
	}

	sscanf(messages, "%x %x", &i2c_reg_addr, &i2c_reg_val);

	RGB_DBG("%s: The register you write, addr = %x, value = %x\n", __func__, i2c_reg_addr, i2c_reg_val);

	rgbSensor_setEnable(true);
	//RGB_DBG("%s: %ld\n", __func__, val);
	ret = tcs3400_i2c_write((u8)i2c_reg_addr, (u8)i2c_reg_val);
	rgbSensor_setEnable(false);

	if(ret < 0)
		return -EFAULT;	

	return len;
	
}

static void create_rgbSensor_register_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  tcs3400_als_register_proc_open,
		.read = seq_read,
		.write = tcs3400_als_register_proc_write,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_regwrite", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}

/* +++Vincent-for debug interface */
struct available_reg {	
	u8 addr;
	char *name;
};

static struct available_reg available_reg_address[] = {
	{0x80, "ENABLE"},
	{0x81, "ATIME"},
	{0x83, "WTIME"},
	{0x84, "AILTL"},
	{0x85, "AILTH"},
	{0x86, "AIHTL"},
	{0x87, "AIHTH"},
	{0x8C, "PERS"},
	{0x8D, "CONFIG"},
	{0x8F, "CONTROL"},
	{0x90, "AUX"},
	{0x91, "REVID"},
	{0x92, "ID"},
	{0x93, "STATUS"},
	{0x94, "CDATAL"},
	{0x95, "CDATAH"},
	{0x96, "RDATAL"},
	{0x97, "RDATAH"},
	{0x98, "GDATAL"},
	{0x99, "GDATAH"},
	{0x9A, "BDATAL"},
	{0x9B, "BDATAH"},
	{0xC0, "IR"},
	{0xE4, "IFORCE"},
	{0xE6, "CICLEAR"},
	{0xE7, "AICLEAR"}
};
static int rgbSensor_dump_proc_read(struct seq_file *buf, void *v)
{
	char *tempout, *tempstr;
	int i;
	u8 val;

	tempstr = kzalloc(sizeof(char)*32, GFP_KERNEL);
	tempout = kzalloc(sizeof(char)*1000, GFP_KERNEL);
	
	snprintf(tempstr, 32, "%8s  addr \tvalue\n", " ");
	strcat(tempout, tempstr);

	rgbSensor_setEnable(true);
	rgbSensor_checkStatus();

	for(i = 0; i < ARRAY_SIZE(available_reg_address); i++){
		tcs3400_i2c_read(available_reg_address[i].addr, &val);
		snprintf(tempstr, 32, "%8s  0x%02x:\t0x%02x\n", available_reg_address[i].name, available_reg_address[i].addr, val);
		strcat(tempout, tempstr);
	}

	rgbSensor_setEnable(false);

	seq_printf(buf, "%s\n", tempout);
	
	kfree(tempstr);
	kfree(tempout);		

	return 0;
}
static int rgbSensor_dump_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_dump_proc_read, NULL);
}
static void create_rgbSensor_dump_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_dump_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_dump", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/* +++Vincent-for debug interface */
/*+++BSP David proc rgbSensor_status Interface+++*/
static int rgbSensor_status_proc_read(struct seq_file *buf, void *v)
{
	int result = 0;
	if (rgbSensor_checkI2C()) {
		result = 1;
	} else{
		result = 0;
	}
	RGB_DBG("%s: %d\n", __func__, result);
	seq_printf(buf, "%d\n", result);
	return 0;
}
static int rgbSensor_status_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_status_proc_read, NULL);
}

static void create_rgbSensor_status_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_status_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_status", 0444, NULL, &proc_fops);
	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc rgbSensor_status Interface---*/
/*+++BSP David proc asusRgbCalibEnable Interface+++*/
static int asusRgbCalibEnable_proc_read(struct seq_file *buf, void *v)
{
	int result = 1;
	seq_printf(buf, "%d\n", result);
	return 0;
}
static int asusRgbCalibEnable_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, asusRgbCalibEnable_proc_read, NULL);
}

static ssize_t asusRgbCalibEnable_proc_write(struct file *filp, const char __user *buff,
		size_t len, loff_t *data)
{
	int val;
	char messages[256];
	if (len > 256) {
		len = 256;
	}
	if (copy_from_user(messages, buff, len)) {
		return -EFAULT;
	}

	val = (int)simple_strtol(messages, NULL, 10);
	RGB_DBG("%s: %d\n", __func__, val);
	return len;
}
static void create_asusRgbCalibEnable_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  asusRgbCalibEnable_proc_open,
		.write = asusRgbCalibEnable_proc_write,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/asusRgbCalibEnable", 0664, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc asusRgbCalibEnable Interface---*/
/*+++BSP David proc asusRgbDebug Interface+++*/
static int asusRgbDebug_proc_read(struct seq_file *buf, void *v)
{
	int result = 0;
	if (g_debugMode) {
		result = 1;
	} else{
		result = 0;
	}
	seq_printf(buf, "%d\n", result);
	return 0;
}
static int asusRgbDebug_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, asusRgbDebug_proc_read, NULL);
}

static ssize_t asusRgbDebug_proc_write(struct file *filp, const char __user *buff,
		size_t len, loff_t *data)
{
	int val;
	char messages[256];
	if (len > 256) {
		len = 256;
	}
	if (copy_from_user(messages, buff, len)) {
		return -EFAULT;
	}

	val = (int)simple_strtol(messages, NULL, 10);
	if (val == 0) {
		g_debugMode = false;
	} else{
		g_debugMode = true;
	}
	RGB_DBG("%s: %d\n", __func__, val);
	return len;
}
static void create_asusRgbDebug_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  asusRgbDebug_proc_open,
		.write = asusRgbDebug_proc_write,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/asusRgbDebug", 0664, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc asusRgbDebug Interface---*/
/*+++BSP David proc asusRgbSetIT Interface+++*/
static int asusRgbSetIT_proc_read(struct seq_file *buf, void *v)
{
	seq_printf(buf, "%d\n", cm_lp_info->it_time);

	return 0;
}
static int asusRgbSetIT_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, asusRgbSetIT_proc_read, NULL);
}

static ssize_t asusRgbSetIT_proc_write(struct file *filp, const char __user *buff,
		size_t len, loff_t *data)
{
	int val;
	char messages[256];
	if (len > 256) {
		len = 256;
	}
	if (copy_from_user(messages, buff, len)) {
		return -EFAULT;
	}

	val = (int)simple_strtol(messages, NULL, 10);
	rgbSensor_itSet_ms(val);
	RGB_DBG("%s: %d\n", __func__, val);
	return len;
}
static void create_asusRgbSetIT_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  asusRgbSetIT_proc_open,
		.write = asusRgbSetIT_proc_write,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/asusRgbSetIT", 0664, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc asusRgbSetIT Interface---*/
#if 0
/*+++BSP David proc rgbSensor_enable Interface+++*/
static ssize_t rgbSensor_enable_proc_write(struct file *filp, const char __user *buff,
		size_t len, loff_t *data)
{
	char messages[256];
	if (len > 256) {
		len = 256;
	}
	if (copy_from_user(messages, buff, len)) {
		return -EFAULT;
	}
	if (messages[0] == '1') {
		rgbSensor_setEnable(true);
	} else{
		rgbSensor_setEnable(false);
	}

	RGB_DBG("%s: %s\n", __func__, messages);
	return len;
}
static int rgbSensor_enable_proc_read(struct seq_file *buf, void *v)
{
	char tempString[16];
	if (rgbSensor_getEnable()) {
		snprintf(tempString, 16, "%s", "enabled");
	} else{
		snprintf(tempString, 16, "%s", "disabled");
	}
	RGB_DBG("%s: %s\n", __func__, tempString);
	return seq_printf(buf, "%s\n", tempString);
}
static int rgbSensor_enable_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_enable_proc_read, NULL);
}
static void create_rgbSensor_enable_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_enable_proc_open,
		.write = rgbSensor_enable_proc_write,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_enable", 0664, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc rgbSensor_enable Interface---*/
/*+++BSP David proc rgbSensor_update_calibration_data Interface+++*/
static int rgbSensor_update_calibration_data_proc_read(struct seq_file *buf, void *v)
{
	int result = 1;
	rgbSensor_updateCalibrationData();
	RGB_DBG("%s: %d\n", __func__, result);
	return seq_printf(buf, "%d\n", result);
}
static int rgbSensor_update_calibration_data_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_update_calibration_data_proc_read, NULL);
}

static void create_rgbSensor_update_calibration_data_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_update_calibration_data_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_update_calibration_data", 0440, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc rgbSensor_update_calibration_data Interface---*/
/*+++BSP David proc rgbSensor_itTime Interface+++*/
static int rgbSensor_itTime_proc_read(struct seq_file *buf, void *v)
{
	int result = -1;
	if (cm_lp_info != NULL) {
		result = cm_lp_info->it_time;
	}
	RGB_DBG("%s: %d\n", __func__, result);
	return seq_printf(buf, "%d\n", result);
}
static int rgbSensor_itTime_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_itTime_proc_read, NULL);
}

static ssize_t rgbSensor_itTime_proc_write(struct file *filp, const char __user *buff,
		size_t len, loff_t *data)
{
	int val;
	char messages[256];
	if (len > 256) {
		len = 256;
	}
	if (copy_from_user(messages, buff, len)) {
		return -EFAULT;
	}

	val = (int)simple_strtol(messages, NULL, 10);
	if (val >= 0 && val <= 5) {
		cm_lp_info->it_time = val;
		if (cm_lp_info != NULL && cm_lp_info->als_enable == 1) {
			RGB_DBG("%s: %d => %x\n", __func__, val, val << 4);
			_tcs3400_I2C_Mask_Write_Word(TCS3400_ADDR, TCS3400_CONF, CFG_IT_MASK, val << 4);
		} else{
			RGB_DBG_E("%s: als not enabled yet, val = %d\n", __func__, val);
		}
	} else{
		RGB_DBG_E("%s : parameter out of range(%d)\n", __func__, val);
	}

	return len;
}
static void create_rgbSensor_itTime_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_itTime_proc_open,
		.write = rgbSensor_itTime_proc_write,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_itTime", 0664, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc rgbSensor_itTime Interface---*/
/*+++BSP David proc rgbSensor_raw_r Interface+++*/
static int rgbSensor_raw_r_proc_read(struct seq_file *buf, void *v)
{
 	u16 adc_data = 0;

	get_rgb_data(RGB_DATA_R, false, &adc_data);
	RGB_DBG("%s: %u\n", __func__, adc_data);
	return seq_printf(buf, "%u\n", adc_data);
}
static int rgbSensor_raw_r_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_raw_r_proc_read, NULL);
}

static void create_rgbSensor_raw_r_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_raw_r_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_raw_r", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc rgbSensor_raw_r Interface---*/
/*+++BSP David proc rgbSensor_raw_g Interface+++*/
static int rgbSensor_raw_g_proc_read(struct seq_file *buf, void *v)
{
 	u16 adc_data = 0;

	get_rgb_data(RGB_DATA_G, false, &adc_data);
	RGB_DBG("%s: %u\n", __func__, adc_data);
	return seq_printf(buf, "%u\n", adc_data);
}
static int rgbSensor_raw_g_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_raw_g_proc_read, NULL);
}

static void create_rgbSensor_raw_g_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_raw_g_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_raw_g", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc rgbSensor_raw_g Interface---*/
/*+++BSP David proc rgbSensor_raw_b Interface+++*/
static int rgbSensor_raw_b_proc_read(struct seq_file *buf, void *v)
{
 	u16 adc_data = 0;

	get_rgb_data(RGB_DATA_B, false, &adc_data);
	RGB_DBG("%s: %u\n", __func__, adc_data);
	return seq_printf(buf, "%u\n", adc_data);
}
static int rgbSensor_raw_b_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_raw_b_proc_read, NULL);
}

static void create_rgbSensor_raw_b_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_raw_b_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_raw_b", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc rgbSensor_raw_b Interface---*/
/*+++BSP David proc rgbSensor_raw_w Interface+++*/
static int rgbSensor_raw_w_proc_read(struct seq_file *buf, void *v)
{
 	u16 adc_data = 0;

	get_rgb_data(RGB_DATA_W, false, &adc_data);
	RGB_DBG("%s: %u\n", __func__, adc_data);
	return seq_printf(buf, "%u\n", adc_data);
}
static int rgbSensor_raw_w_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_raw_w_proc_read, NULL);
}

static void create_rgbSensor_raw_w_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_raw_w_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_raw_w", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc rgbSensor_raw_w Interface---*/
/*+++BSP David proc rgbSensor_r Interface+++*/
static int rgbSensor_r_proc_read(struct seq_file *buf, void *v)
{
 	u16 adc_data = 0;

	get_rgb_data(RGB_DATA_R, true, &adc_data);
	RGB_DBG("%s: %u\n", __func__, adc_data);
	return seq_printf(buf, "%u\n", adc_data);
}
static int rgbSensor_r_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_r_proc_read, NULL);
}

static void create_rgbSensor_r_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_r_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_r", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc rgbSensor_r Interface---*/
/*+++BSP David proc rgbSensor_g Interface+++*/
static int rgbSensor_g_proc_read(struct seq_file *buf, void *v)
{
 	u16 adc_data = 0;

	get_rgb_data(RGB_DATA_G, true, &adc_data);
	RGB_DBG("%s: %u\n", __func__, adc_data);
	return seq_printf(buf, "%u\n", adc_data);
}
static int rgbSensor_g_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_g_proc_read, NULL);
}

static void create_rgbSensor_g_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_g_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_g", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc rgbSensor_g Interface---*/
/*+++BSP David proc rgbSensor_b Interface+++*/
static int rgbSensor_b_proc_read(struct seq_file *buf, void *v)
{
 	u16 adc_data = 0;

	get_rgb_data(RGB_DATA_B, true, &adc_data);
	RGB_DBG("%s: %u\n", __func__, adc_data);
	return seq_printf(buf, "%u\n", adc_data);
}
static int rgbSensor_b_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_b_proc_read, NULL);
}

static void create_rgbSensor_b_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_b_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_b", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc rgbSensor_b Interface---*/
/*+++BSP David proc rgbSensor_w Interface+++*/
static int rgbSensor_w_proc_read(struct seq_file *buf, void *v)
{
 	u16 adc_data = 0;

	get_rgb_data(RGB_DATA_W, true, &adc_data);
	RGB_DBG("%s: %u\n", __func__, adc_data);
	return seq_printf(buf, "%u\n", adc_data);
}
static int rgbSensor_w_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rgbSensor_w_proc_read, NULL);
}

static void create_rgbSensor_w_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  rgbSensor_w_proc_open,
		.read = seq_read,
		.release = single_release,
	};
	struct proc_dir_entry *proc_file = proc_create("driver/rgbSensor_w", 0444, NULL, &proc_fops);

	if (!proc_file) {
		RGB_DBG_E("[Proc]%s failed!\n", __FUNCTION__);
	}
	return;
}
/*---BSP David proc rgbSensor_w Interface---*/
#endif
/*---BSP David ASUS Interface---*/


static int rgbSensor_miscOpen(struct inode *inode, struct file *file)
{
	int ret = 0;
	ret = rgbSensor_setEnable(true);
	RGB_DBG("%s: %d\n", __func__, ret);
	if (ret < 0) {
		rgbSensor_setEnable(false);
	}
	return ret;
}

static int rgbSensor_miscRelease(struct inode *inode, struct file *file)
{
	int ret = 0;
	ret = rgbSensor_setEnable(false);
	RGB_DBG("%s: %d\n", __func__, ret);
	return ret;
}
static long rgbSensor_miscIoctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int it_time = 0;
	int ret = 0;
	int dataI[ASUS_RGB_SENSOR_DATA_SIZE];
	char dataS[ASUS_RGB_SENSOR_NAME_SIZE];
 	uint16_t adc_data;
	u8 adc_status;
	int l_debug_mode = 0;
	switch (cmd) {
		case ASUS_RGB_SENSOR_IOCTL_IT_SET:
			if (cm_lp_info->als_enable != 1) {
				ret = -2;
				RGB_DBG_E("%s: als not enabled yet!\n", __func__);
				goto end;
			}
			ret = copy_from_user(&it_time, (int __user*)arg, sizeof(it_time));
			if (ret < 0) {
				RGB_DBG_E("%s: cmd = IT_SET, copy_from_user error(%d)\n", __func__, it_time);
				goto end;
			}
			ret = rgbSensor_itSet_ms(it_time);
			RGB_DBG_API("%s: cmd = IT_SET, time = %dms\n", __func__, cm_lp_info->it_time);
			break;
		case ASUS_RGB_SENSOR_IOCTL_DATA_READ:
			/* +++Vincent- workaround for reading wrong value */
			ret = tcs3400_i2c_read(TCS3400_STATUS, &adc_status);
			if (ret < 0) {
				goto end;
			}
			
			ret = get_rgb_data(RGB_DATA_R, &adc_data, true);
			if (ret < 0) {
				goto end;
			}
			dataI[0] = adc_data;
			ret = get_rgb_data(RGB_DATA_G, &adc_data, true);
			if (ret < 0) {
				goto end;
			}
			dataI[1] = adc_data;
			ret = get_rgb_data(RGB_DATA_B, &adc_data, true);
			if (ret < 0) {
				goto end;
			}
			dataI[2] = adc_data;
			ret = get_rgb_data(RGB_DATA_W, &adc_data, true);
			if (ret < 0) {
				goto end;
			}
			dataI[3] = adc_data;
			dataI[4] = 1;
			RGB_DBG_API("%s: cmd = DATA_READ, data[0] = %d, data[1] = %d, data[2] = %d, data[3] = %d, data[4] = %d\n"
				, __func__, dataI[0], dataI[1], dataI[2], dataI[3], dataI[4]);
			ret = copy_to_user((int __user*)arg, &dataI, sizeof(dataI));
			break;
		case ASUS_RGB_SENSOR_IOCTL_MODULE_NAME:
			snprintf(dataS, sizeof(dataS), "%s", TCS3400_I2C_NAME);
			RGB_DBG_API("%s: cmd = MODULE_NAME, name = %s\n", __func__, dataS);
			ret = copy_to_user((int __user*)arg, &dataS, sizeof(dataS));
			break;
		case ASUS_RGB_SENSOR_IOCTL_DEBUG_MODE:
			if (g_debugMode) {
				l_debug_mode = 1;
			} else{
				l_debug_mode = 0;
			}
			RGB_DBG_API("%s: cmd = DEBUG_MODE, result = %d\n", __func__, l_debug_mode);
			ret = copy_to_user((int __user*)arg, &l_debug_mode, sizeof(l_debug_mode));
			break;
		default:
			ret = -1;
			RGB_DBG_E("%s: default\n", __func__);
	}
end:
	return ret;
}
static struct file_operations tcs3400_fops = {
  .owner = THIS_MODULE,
  .open = rgbSensor_miscOpen,
  .release = rgbSensor_miscRelease,
  .unlocked_ioctl = rgbSensor_miscIoctl,
  .compat_ioctl = rgbSensor_miscIoctl
};
struct miscdevice tcs3400_misc = {
  .minor = MISC_DYNAMIC_MINOR,
  .name = "asusRgbSensor",
  .fops = &tcs3400_fops
};
static int rgbSensor_miscRegister(void)
{
	int rtn = 0;
	rtn = misc_register(&tcs3400_misc);
	if (rtn < 0) {
		RGB_DBG_E("[%s] Unable to register misc deive\n", __func__);
		misc_deregister(&tcs3400_misc);
	}
	return rtn;
}
//+++Vincent
#if 0
static int32_t rgbSensor_getDt(struct device_node *of_node,
		struct tcs3400_info *fctrl)
{
	struct msm_rgb_sensor_vreg *vreg_cfg = NULL;
	int rc;

	if (of_find_property(of_node,
			"qcom,cam-vreg-name", NULL)) {
		vreg_cfg = &fctrl->vreg_cfg;
		rc = msm_camera_get_dt_vreg_data(of_node,
			&vreg_cfg->cam_vreg, &vreg_cfg->num_vreg);
		if (rc < 0) {
			RGB_DBG_E("%s: failed rc %d\n", __func__, rc);
		}
	} else{
		RGB_DBG_E("%s: can't find regulator\n", __func__);
		rc = -1;
	}
	return rc;
}
#endif
//---Vincent

#ifdef CONFIG_I2C_STRESS_TEST
static int TestRgbSensorReadWrite(struct i2c_client *client)
{
	bool statusValid = false;
	i2c_log_in_test_case("[TCS3400][Test]%s start\n", __func__);
	statusValid = rgbSensor_checkI2C();
	i2c_log_in_test_case("[TCS3400][Test]%s: %s\n", __func__, statusValid ? "PASS" : "FAIL");
	return statusValid ? I2C_TEST_PASS : I2C_TEST_TCS3400_FAIL;
};
static struct i2c_test_case_info gRgbSensorTestCaseInfo[] =
{
	__I2C_STRESS_TEST_CASE_ATTR(TestRgbSensorReadWrite),
};
static void tcs3400_i2c_stress_test(void)
{
	RGB_DBG("%s\n", __FUNCTION__);
	i2c_add_test_case(cm_lp_info->i2c_client, "tcs3400", ARRAY_AND_SIZE(gRgbSensorTestCaseInfo));
}
#endif
static int tcs3400_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct tcs3400_info *lpi;

	RGB_DBG("%s start\n", __func__);

	lpi = kzalloc(sizeof(struct tcs3400_info), GFP_KERNEL);
	if (!lpi) {
		RGB_DBG_E("%s: fail to allocate tcs3400_info!\n", __func__);
		return -ENOMEM;
	}

	lpi->i2c_client = client;
	i2c_set_clientdata(client, lpi);
	
	lpi->power = NULL; //if necessary, add power function here for sensor chip
	lpi->polling_delay = msecs_to_jiffies(LS_POLLING_DELAY);
	lpi->using_calib = true;
	cm_lp_info = lpi;
	g_debugMode = false;
	//+++Vincent
	//rgbSensor_getDt(client->dev.of_node, lpi);
	//---Vincent
	mutex_init(&als_enable_mutex);
	ret = rgbSensor_setup(lpi);
	if (ret < 0) {
		RGB_DBG_E("%s: rgbSensor_setup error!\n", __func__);
		goto err_rgbSensor_setup;
	}
	ret = rgbSensor_miscRegister();
	if (ret < 0) {
		RGB_DBG_E("%s: rgbSensor_miscRegister error!\n", __func__);
		goto err_rgbSensor_miscRegister;
	}

#ifdef CONFIG_I2C_STRESS_TEST
	tcs3400_i2c_stress_test();
#endif
	create_rgbSensor_dump_proc_file();
	create_rgbSensor_status_proc_file();
	create_asusRgbCalibEnable_proc_file();
	create_asusRgbDebug_proc_file();
	create_asusRgbSetIT_proc_file();

/* +++Vincent */
	create_rgbSensor_showrgbc_proc_file();
	create_rgbSensor_gain_proc_file();
	create_rgbSensor_register_proc_file();
	create_rgbSensor_enable_proc_file();
/* ---Vincent */

#if 0
	create_rgbSensor_enable_proc_file();
	create_rgbSensor_itTime_proc_file();
	create_rgbSensor_update_calibration_data_proc_file();
	create_rgbSensor_r_proc_file();
	create_rgbSensor_g_proc_file();
	create_rgbSensor_b_proc_file();
	create_rgbSensor_w_proc_file();
	create_rgbSensor_raw_r_proc_file();
	create_rgbSensor_raw_g_proc_file();
	create_rgbSensor_raw_b_proc_file();
	create_rgbSensor_raw_w_proc_file();
#endif

	RGB_DBG("%s: Probe success!\n", __func__);
	
	return ret;

err_rgbSensor_miscRegister:
err_rgbSensor_setup:
	mutex_destroy(&als_enable_mutex);
	kfree(lpi);
	cm_lp_info = NULL;
	return ret;
}

static int tcs3400_remove(struct i2c_client *client)
{
	misc_deregister(&tcs3400_misc);
	return 0;
}
static const struct i2c_device_id tcs3400_i2c_id[] = {
	{TCS3400_I2C_NAME, 0},
	{}
};

#ifdef CONFIG_OF
  static struct of_device_id tcs3400_match_table[] = {
          { .compatible = RGB_SENSOR_COMPATIBLE_NAME,},
          { },
  };
#else
  #define tcs3400_match_table NULL
#endif

static struct i2c_driver tcs3400_driver = {
	.id_table = tcs3400_i2c_id,
	.probe = tcs3400_probe,
	.remove    =tcs3400_remove,
	.driver = {
		.name = TCS3400_I2C_NAME,
		.owner = THIS_MODULE,
 		.of_match_table = of_match_ptr(tcs3400_match_table),
	},
};

static int __init tcs3400_init(void)
{
	//mutex_init(&g_i2c_lock);
	return i2c_add_driver(&tcs3400_driver);
}

static void __exit tcs3400_exit(void)
{
	//mutex_destroy(&g_i2c_lock);
	i2c_del_driver(&tcs3400_driver);
}

module_init(tcs3400_init);
module_exit(tcs3400_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("RGB sensor device driver for TCS3400");
MODULE_AUTHOR("Frank Hsieh <frank.hsieh@vishay.com>");
