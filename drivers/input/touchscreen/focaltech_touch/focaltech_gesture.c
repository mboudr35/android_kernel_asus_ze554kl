/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2016, Focaltech Ltd. All rights reserved.
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

/*****************************************************************************
*
* File Name: focaltech_gestrue.c
*
* Author: Focaltech Driver Team
*
* Created: 2016-08-08
*
* Abstract:
*
* Reference:
*
*****************************************************************************/

/*****************************************************************************
* 1.Included header files
*****************************************************************************/
#include "focaltech_core.h"

#if FTS_GESTURE_EN
/******************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define  KEY_GESTURE_U                          KEY_U
#define  KEY_GESTURE_UP                         KEY_UP
#define  KEY_GESTURE_DOWN                       KEY_DOWN
#define  KEY_GESTURE_LEFT                       KEY_LEFT
#define  KEY_GESTURE_RIGHT                      KEY_RIGHT
#define  KEY_GESTURE_O                          KEY_O
#define  KEY_GESTURE_E                          KEY_E
#define  KEY_GESTURE_M                          KEY_M
#define  KEY_GESTURE_L                          KEY_L
#define  KEY_GESTURE_W                          KEY_W
#define  KEY_GESTURE_S                          KEY_S
#define  KEY_GESTURE_V                          KEY_V
#define  KEY_GESTURE_C                          KEY_C
#define  KEY_GESTURE_Z                          KEY_Z
#define  KEY_GESTURE_Heart   					KEY_L

#define GESTURE_LEFT                            0x20
#define GESTURE_RIGHT                           0x21
#define GESTURE_UP                              0x22
#define GESTURE_DOWN                            0x23
#define GESTURE_DOUBLECLICK                     0x24
#define GESTURE_O                               0x30
#define GESTURE_W                               0x31
#define GESTURE_M                               0x32
#define GESTURE_E                               0x33
#define GESTURE_L                               0x44
#define GESTURE_S                               0x46
#define GESTURE_V                               0x54
#define GESTURE_Z                               0x65
#define GESTURE_C                               0x34
#define GESTURE_Heart							0x50
#define FTS_GESTRUE_POINTS                      255
#define FTS_GESTRUE_POINTS_HEADER               8
#define FTS_GESTURE_OUTPUT_ADRESS               0xD3
/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/
struct fts_gesture_st
{
    u8 header[FTS_GESTRUE_POINTS_HEADER];
    u16 coordinate_x[FTS_GESTRUE_POINTS];
    u16 coordinate_y[FTS_GESTRUE_POINTS];
    u8 mode;
    u8 active;              /* 1-enter into gesture(suspend) 0-gesture disable or LCD on */
};

/*****************************************************************************
* Static variables
*****************************************************************************/
static struct fts_gesture_st fts_gesture_data;
extern struct fts_ts_data *fts_wq_data;
extern u8 FTS_gesture_register_d1;
extern u8 FTS_gesture_register_d2;
extern u8 FTS_gesture_register_d5;
extern u8 FTS_gesture_register_d6;
extern u8 FTS_gesture_register_d7;
/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
extern bool proximity_check_status(void);
/*****************************************************************************
* Static function prototypes
*****************************************************************************/
/*static ssize_t fts_gesture_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t fts_gesture_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);*/
static ssize_t switch_gesture_mode_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t switch_gesture_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t fts_gesture_buf_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t fts_gesture_buf_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t switch_dclick_mode_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t switch_dclick_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t switch_swipeup_mode_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t switch_swipeup_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);



/* sysfs gesture node
 *   read example: cat  fts_gesture_mode        ---read gesture mode
 *   write example:echo 01 > fts_gesture_mode   ---write gesture mode to 01
 *
 */
static DEVICE_ATTR (gesture_mode, S_IRUGO|S_IWUSR, switch_gesture_mode_show, switch_gesture_mode_store);
/*
 *   read example: cat  fts_gesture_buf        ---read gesture buf
 */
static DEVICE_ATTR (fts_gesture_buf, S_IRUGO|S_IWUSR, fts_gesture_buf_show, fts_gesture_buf_store);

static DEVICE_ATTR (dclick_mode, S_IRUGO|S_IWUSR, switch_dclick_mode_show, switch_dclick_mode_store);
static DEVICE_ATTR (swipeup_mode, S_IRUGO|S_IWUSR, switch_swipeup_mode_show, switch_swipeup_mode_store);


static struct attribute *fts_gesture_mode_attrs[] =
{

    &dev_attr_gesture_mode.attr,
    &dev_attr_fts_gesture_buf.attr,
    &dev_attr_dclick_mode.attr,
    &dev_attr_swipeup_mode.attr,
    NULL,
};

static struct attribute_group fts_gesture_group =
{
    .attrs = fts_gesture_mode_attrs,
};


/************************************************************************
* Name: fts_gesture_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return:
***********************************************************************/
/*static ssize_t fts_gesture_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int count;
    u8 val;
    struct i2c_client *client = container_of(dev, struct i2c_client, dev);

    mutex_lock(&fts_input_dev->mutex);
    fts_i2c_read_reg(client, FTS_REG_GESTURE_EN, &val);
    count = sprintf(buf, "Gesture Mode: %s\n", fts_gesture_data.mode ? "On" : "Off");
    count += sprintf(buf + count, "Reg(0xD0) = %d\n", val);
    mutex_unlock(&fts_input_dev->mutex);

    return count;
}*/

/************************************************************************
* Name: fts_gesture_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return:
***********************************************************************/
/*static ssize_t fts_gesture_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mutex_lock(&fts_input_dev->mutex);

	if (FTS_SYSFS_ECHO_ON(buf))
	{
		FTS_INFO("[GESTURE]enable gesture");
        fts_gesture_data.mode = ENABLE;
	}
	else if (FTS_SYSFS_ECHO_OFF(buf))
	{
		FTS_INFO("[GESTURE]disable gesture");
        fts_gesture_data.mode = DISABLE;
	}

    mutex_unlock(&fts_input_dev->mutex);


    return count;
}*/
static ssize_t switch_gesture_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp = 0;
	u8 gesturetmp = 0;
	char gesture_buf[16];
	char cmpchar = '1';

	memset(gesture_buf, 0, sizeof(gesture_buf));
	sprintf(gesture_buf, "%s", buf);
	gesture_buf[count-1] = '\0';

	pr_err("[Focal][Touch] gesture_mode_store %s ! \n", gesture_buf);

	if (gesture_buf[0] == cmpchar) {
		fts_wq_data->gesture_mode_eable = true;
		printk("[Focal][Touch] gesture_mode enable ! \n");
	} else
		fts_wq_data->gesture_mode_eable = false;

	if (fts_wq_data->gesture_mode_eable == 1) {

		for (tmp = 0; tmp < 7; tmp++) {
			if (gesture_buf[tmp] == cmpchar) {
				gesturetmp |= (1 << tmp);
			}
		}

		fts_wq_data->gesture_mode_type = gesturetmp;

		if ((fts_wq_data->gesture_mode_type & 1 << 6))
			FTS_gesture_register_d6 = 0x10;
		else
			FTS_gesture_register_d6 = 0x00;

		if ((fts_wq_data->gesture_mode_type & 1 << 5))
			FTS_gesture_register_d7 = 0x20;
		else
			FTS_gesture_register_d7 = 0x0;

		if ((fts_wq_data->gesture_mode_type & 1 << 4))
			FTS_gesture_register_d2 |= 0x10;
		else
			FTS_gesture_register_d2 &= 0xef;

		if ((fts_wq_data->gesture_mode_type & 1 << 3))
			FTS_gesture_register_d2 |= 0x08;
		else
			FTS_gesture_register_d2 &= 0xf7;

		if ((fts_wq_data->gesture_mode_type & 1 << 2))
			FTS_gesture_register_d5 |= 0x40;
		else
			FTS_gesture_register_d5 &= 0xbf;

		if ((fts_wq_data->gesture_mode_type & 1 << 1))
			FTS_gesture_register_d2 |= 0x02;
		else
			FTS_gesture_register_d2 &= 0xfd;

		printk("[Focal][Touch] gesture_mode_enable type = %x ! \n", fts_wq_data->gesture_mode_type);

	} else {

		fts_wq_data->gesture_mode_eable = 0;

		fts_wq_data->gesture_mode_type = 0;

		FTS_gesture_register_d2 = 0;

		FTS_gesture_register_d6 = 0;

		FTS_gesture_register_d7 = 0;

		printk("[Focal][Touch] gesture_mode_disable ! \n");
		}

	return count;

}
static ssize_t switch_gesture_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	bool tmp = 0;

	tmp = (fts_wq_data->gesture_mode_type >> 6) & 1;

	if (!tmp)
		return sprintf(buf, "%x \n", tmp);
	else
		return sprintf(buf, "%x \n", fts_wq_data->gesture_mode_type);

}


static ssize_t switch_dclick_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp = 0;

	tmp = buf[0] - 48;

	if (tmp == 0) {

		fts_wq_data->dclick_mode_eable = 0;

		printk("[Focal][Touch] dclick_mode_disable ! \n");

	} else if (tmp == 1) {

		fts_wq_data->dclick_mode_eable = 1;

		printk("[Focal][Touch] dclick_mode_enable ! \n");
	}

	return count;

}
static ssize_t switch_dclick_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "%d \n", fts_wq_data->dclick_mode_eable);
}
static ssize_t switch_swipeup_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp = 0;

	tmp = buf[0] - 48;

	if (tmp == 0) {

		fts_wq_data->swipeup_mode_eable = 0;

		printk("[Focal][Touch] swipeup_mode_disable ! \n");

	} else if (tmp == 1) {

		fts_wq_data->swipeup_mode_eable = 1;

		printk("[Focal][Touch] swipeup_mode_enable ! \n");
	}

	return count;

}

static ssize_t switch_swipeup_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "%d \n", fts_wq_data->swipeup_mode_eable);
}


/************************************************************************
* Name: fts_gesture_buf_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return:
***********************************************************************/
static ssize_t fts_gesture_buf_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int count;
    int i = 0;

    mutex_lock(&fts_input_dev->mutex);
    count = snprintf(buf, PAGE_SIZE, "Gesture ID: 0x%x\n", fts_gesture_data.header[0]);
    count += snprintf(buf + count, PAGE_SIZE, "Gesture PointNum: %d\n", fts_gesture_data.header[1]);
    count += snprintf(buf + count, PAGE_SIZE, "Gesture Point Buf:\n");
    for (i = 0; i < fts_gesture_data.header[1]; i++)
    {
        count += snprintf(buf + count, PAGE_SIZE, "%3d(%4d,%4d) ", i, fts_gesture_data.coordinate_x[i], fts_gesture_data.coordinate_y[i]);
        if ((i + 1)%4 == 0)
            count += snprintf(buf + count, PAGE_SIZE, "\n");
    }
    count += snprintf(buf + count, PAGE_SIZE, "\n");
    mutex_unlock(&fts_input_dev->mutex);

    return count;
}

/************************************************************************
* Name: fts_gesture_buf_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return:
***********************************************************************/
static ssize_t fts_gesture_buf_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    /* place holder for future use */
    return -EPERM;
}

/*****************************************************************************
*   Name: fts_create_gesture_sysfs
*  Brief:
*  Input:
* Output: None
* Return: 0-success or error
*****************************************************************************/
int fts_create_gesture_sysfs(struct i2c_client *client)
{
    int ret = 0;

    ret = sysfs_create_group(&client->dev.kobj, &fts_gesture_group);
    if ( ret != 0)
    {
        FTS_ERROR( "[GESTURE]fts_gesture_mode_group(sysfs) create failed!");
        sysfs_remove_group(&client->dev.kobj, &fts_gesture_group);
        return ret;
    }
    return 0;
}

/*****************************************************************************
*   Name: fts_gesture_recovery
*  Brief: recovery gesture state when reset
*  Input:
* Output: None
* Return:
*****************************************************************************/
void fts_gesture_recovery(struct i2c_client *client)
{
    if (fts_gesture_data.mode && fts_gesture_data.active)
    {
        fts_i2c_write_reg(client, FTS_REG_GESTURE_EN, ENABLE);
        fts_i2c_write_reg(fts_i2c_client, 0xD1, 0xff);
        fts_i2c_write_reg(fts_i2c_client, 0xD2, 0xff);
        fts_i2c_write_reg(fts_i2c_client, 0xD5, 0xff);
        fts_i2c_write_reg(fts_i2c_client, 0xD6, 0xff);
        fts_i2c_write_reg(fts_i2c_client, 0xD7, 0xff);
        fts_i2c_write_reg(fts_i2c_client, 0xD8, 0xff);
    }
}

/*****************************************************************************
*   Name: fts_gesture_init
*  Brief:
*  Input:
* Output: None
* Return: None
*****************************************************************************/
int fts_gesture_init(struct input_dev *input_dev, struct i2c_client *client)
{
    FTS_FUNC_ENTER();
    input_set_capability(input_dev, EV_KEY, KEY_POWER);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_U);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_UP);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_DOWN);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_LEFT);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_RIGHT);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_O);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_E);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_M);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_L);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_W);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_S);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_V);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_Z);
    input_set_capability(input_dev, EV_KEY, KEY_GESTURE_C);

    __set_bit(KEY_GESTURE_RIGHT, input_dev->keybit);
    __set_bit(KEY_GESTURE_LEFT, input_dev->keybit);
    __set_bit(KEY_GESTURE_UP, input_dev->keybit);
    __set_bit(KEY_GESTURE_DOWN, input_dev->keybit);
    __set_bit(KEY_GESTURE_U, input_dev->keybit);
    __set_bit(KEY_GESTURE_O, input_dev->keybit);
    __set_bit(KEY_GESTURE_E, input_dev->keybit);
    __set_bit(KEY_GESTURE_M, input_dev->keybit);
    __set_bit(KEY_GESTURE_W, input_dev->keybit);
    __set_bit(KEY_GESTURE_L, input_dev->keybit);
    __set_bit(KEY_GESTURE_S, input_dev->keybit);
    __set_bit(KEY_GESTURE_V, input_dev->keybit);
    __set_bit(KEY_GESTURE_C, input_dev->keybit);
    __set_bit(KEY_GESTURE_Z, input_dev->keybit);

    fts_create_gesture_sysfs(client);
    fts_gesture_data.mode = 0;
    fts_gesture_data.active = 0;
    FTS_FUNC_EXIT();
    return 0;
}

/************************************************************************
* Name: fts_gesture_exit
* Brief:  remove sys
* Input: i2c info
* Output: no
* Return: no
***********************************************************************/
int fts_gesture_exit(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &fts_gesture_group);
    return 0;
}

/*****************************************************************************
*   Name: fts_check_gesture
*  Brief:
*  Input:
* Output: None
* Return: None
*****************************************************************************/
static void fts_check_gesture(struct input_dev *input_dev,int gesture_id)
{
    char *envp[2];
    int gesture;
	bool Ps_status = false;
	pr_err("[Focal][Touch] %s :  gesture_id = 0x%x\n ", __func__, gesture_id);

	if (!fts_wq_data->cover_mode_eable)
	{	
		Ps_status = proximity_check_status();
		pr_err("[touch]check proximity status  \n");
	}

    pr_err("[focal][touch]gesutre id = %x \n",gesture_id);
	if(!Ps_status){
		switch (gesture_id)
    	{
        	case GESTURE_LEFT:
            	envp[0]="GESTURE=LEFT";
            	gesture = KEY_GESTURE_LEFT;
            	break;
        	case GESTURE_RIGHT:
            	envp[0]="GESTURE=RIGHT";
            	gesture = KEY_GESTURE_RIGHT;
            	break;
        	case GESTURE_UP:
            	envp[0]="GESTURE=UP";
            	gesture = KEY_GESTURE_UP;
            	break;
        	case GESTURE_DOWN:
            	envp[0]="GESTURE=DOWN";
            	gesture = KEY_GESTURE_DOWN;
            	break;
        	case GESTURE_DOUBLECLICK:
            	envp[0]="GESTURE=DOUBLE_CLICK";
            	gesture = KEY_POWER;
            	break;
        	case GESTURE_O:
           	 	envp[0]="GESTURE=O";
           	 	gesture = KEY_GESTURE_O;
            	break;
        	case GESTURE_W:
            	envp[0]="GESTURE=W";
            	gesture = KEY_GESTURE_W;
            	break;
        	case GESTURE_M:
            	envp[0]="GESTURE=M";
            	gesture = KEY_GESTURE_M;
            	break;
        	case GESTURE_E:
            	envp[0]="GESTURE=E";
            	gesture = KEY_GESTURE_E;
            	break;
        	case GESTURE_L:
           	 	envp[0]="GESTURE=L";
            	gesture = KEY_GESTURE_L;
            	break;
        	case GESTURE_S:
            	envp[0]="GESTURE=S";
            	gesture = KEY_GESTURE_S;
            	break;
        	case GESTURE_V:
            	envp[0]="GESTURE=V";
            	gesture = KEY_GESTURE_V;
            	break;
        	case GESTURE_Z:
            	envp[0]="GESTURE=Z";
            	gesture = KEY_GESTURE_Z;
            	break;
        	case  GESTURE_C:
            	envp[0]="GESTURE=C";
            	gesture = KEY_GESTURE_C;
            	break;
			case  GESTURE_Heart:
            	envp[0]="GESTURE=DOUBLE_CLICK";
            	gesture = KEY_POWER;
            	break;
        	default:
            	envp[0]="GESTURE=NONE";
            	gesture = -1;
            	break;
    	}
    	FTS_DEBUG("envp[0]: %s", envp[0]);
    	/* report event key */
    	if (gesture != -1)
    	{
        	input_report_key(input_dev, gesture, 1);
        	input_sync(input_dev);
        	input_report_key(input_dev, gesture, 0);
        	input_sync(input_dev);
    	}
		envp[1]=NULL;
    	kobject_uevent_env(&fts_wq_data->client->dev.kobj, KOBJ_CHANGE, envp);
    	sysfs_notify(&fts_wq_data->client->dev.kobj, NULL, "GESTURE_ID");
	} else {
		pr_err("[Focal][Touch] %s :  Skip wake up devices !\n ", __func__);
	  }
    

    FTS_FUNC_EXIT();
}

/************************************************************************
*   Name: fts_gesture_readdata
* Brief: read data from TP register
* Input: no
* Output: no
* Return: fail <0
***********************************************************************/
static int fts_gesture_read_buffer(struct i2c_client *client, u8 *buf, int read_bytes)
{
    int remain_bytes;
    int ret;
    int i;

    if (read_bytes <= I2C_BUFFER_LENGTH_MAXINUM)
    {
        ret = fts_i2c_read(client, buf, 1, buf, read_bytes);
    }
    else
    {
        ret = fts_i2c_read(client, buf, 1, buf, I2C_BUFFER_LENGTH_MAXINUM);
        remain_bytes = read_bytes - I2C_BUFFER_LENGTH_MAXINUM;
        for (i = 1; remain_bytes > 0; i++)
        {
            if (remain_bytes <= I2C_BUFFER_LENGTH_MAXINUM)
                ret = fts_i2c_read(client, buf, 0, buf + I2C_BUFFER_LENGTH_MAXINUM * i, remain_bytes);
            else
                ret = fts_i2c_read(client, buf, 0, buf + I2C_BUFFER_LENGTH_MAXINUM * i, I2C_BUFFER_LENGTH_MAXINUM);
            remain_bytes -= I2C_BUFFER_LENGTH_MAXINUM;
        }
    }

    return ret;
}

/************************************************************************
*   Name: fts_gesture_readdata
* Brief: read data from TP register
* Input: no
* Output: no
* Return: fail <0
***********************************************************************/
int fts_gesture_readdata(struct i2c_client *client)
{
    u8 buf[FTS_GESTRUE_POINTS * 4] = { 0 };
    int ret = -1;
    int i = 0;
    int gestrue_id = 0;
    int read_bytes = 0;
    u8 pointnum;

    FTS_FUNC_ENTER();
    /* init variable before read gesture point */
    memset(fts_gesture_data.header, 0, FTS_GESTRUE_POINTS_HEADER);
    memset(fts_gesture_data.coordinate_x, 0, FTS_GESTRUE_POINTS * sizeof(u16));
    memset(fts_gesture_data.coordinate_y, 0, FTS_GESTRUE_POINTS * sizeof(u16));

    buf[0] = FTS_REG_GESTURE_OUTPUT_ADDRESS;
    ret = fts_i2c_read(client, buf, 1, buf, FTS_GESTRUE_POINTS_HEADER);
    if (ret < 0)
    {
        FTS_ERROR("[GESTURE]Read gesture header data failed!!");
        FTS_FUNC_EXIT();
        return ret;
    }

    /* FW recognize gesture */
    if (chip_types.chip_idh == 0x54 || chip_types.chip_idh == 0x58 || chip_types.chip_idh == 0x86 || chip_types.chip_idh == 0x87 || chip_types.chip_idh == 0x64)
    {
        memcpy(fts_gesture_data.header, buf, FTS_GESTRUE_POINTS_HEADER);
        gestrue_id = buf[0];
        pointnum = buf[1];
        read_bytes = ((int)pointnum) * 4 + 2;
        buf[0] = FTS_REG_GESTURE_OUTPUT_ADDRESS;
        FTS_DEBUG("[GESTURE]PointNum=%d", pointnum);
        ret = fts_gesture_read_buffer(client, buf, read_bytes);
        if (ret < 0)
        {
            pr_err("[focal][touch][GESTURE]Read gesture touch data failed!!\n");
            FTS_FUNC_EXIT();
            return ret;
        }

        fts_check_gesture(fts_input_dev,gestrue_id);
        for (i = 0; i < pointnum; i++)
        {
            fts_gesture_data.coordinate_x[i] = (((s16) buf[0 + (4 * i + 2)]) & 0x0F) << 8
                                               | (((s16) buf[1 + (4 * i + 2)]) & 0xFF);
            fts_gesture_data.coordinate_y[i] = (((s16) buf[2 + (4 * i + 2)]) & 0x0F) << 8
                                               | (((s16) buf[3 + (4 * i + 2)]) & 0xFF);
        }
        return 0;
    }
    /* other IC's gestrue in driver */
    if (0x24 == buf[0])
    {
        gestrue_id = 0x24;
        fts_check_gesture(fts_input_dev,gestrue_id);
        pr_err("[GESTURE]%d check_gesture gestrue_id", gestrue_id);
        FTS_FUNC_EXIT();
        return -1;
    }

    /* Host Driver recognize gesture */
    pointnum = buf[1];
    read_bytes = ((int)pointnum) * 4 + 2;
    buf[0] = FTS_REG_GESTURE_OUTPUT_ADDRESS;
    ret = fts_gesture_read_buffer(client, buf, read_bytes);
    if (ret < 0)
    {
        FTS_ERROR("[GESTURE]Driver recognize gesture - Read gesture touch data failed!!");
        FTS_FUNC_EXIT();
        return ret;
    }

    /*
    * Host Driver recognize gesture, need gesture lib.a
    * Not use now for compatibility
    gestrue_id = fetch_object_sample(buf, pointnum);
    */
    gestrue_id = 0x24;
    fts_check_gesture(fts_input_dev, gestrue_id);
    FTS_DEBUG("[GESTURE]%d read gestrue_id", gestrue_id);

    for (i = 0; i < pointnum; i++)
    {
        fts_gesture_data.coordinate_x[i] =  (((s16) buf[0 + (4 * i+8)]) & 0x0F) << 8
                                            | (((s16) buf[1 + (4 * i+8)])& 0xFF);
        fts_gesture_data.coordinate_y[i] = (((s16) buf[2 + (4 * i+8)]) & 0x0F) << 8
                                           | (((s16) buf[3 + (4 * i+8)]) & 0xFF);
    }
    FTS_FUNC_EXIT();
    return -1;
}

/*****************************************************************************
*   Name: fts_gesture_suspend
*  Brief:
*  Input:
* Output: None
* Return: None
*****************************************************************************/
int fts_gesture_suspend(struct i2c_client *i2c_client)
{
    int i;
    int state=0;
//	if ((fts_wq_data->dclick_mode_eable == 1) | (fts_wq_data->swipeup_mode_eable == 1) | (fts_wq_data->gesture_mode_eable == 1)) {
	pr_err("[Focal][Touch] %s : Touch gesture mode \n", __func__);
	/*for (i = 0; i < 5; i++)
    {
        state=fts_i2c_write_reg(i2c_client, FTS_REG_GESTURE_EN, 0x01);
		if (state <0){
						msleep(10);
						pr_err("[FTS][tocuh] write d0 fail %d times \n",i);
		}
       	else
            break;
    }
	msleep(10);*/
	fts_gesture_data.mode = 1;
			if (fts_wq_data->gesture_mode_eable == 1) {
				pr_err("[Focal][Touch] %s : open zenmotion mode =1 \n", __func__);
				if (fts_wq_data->dclick_mode_eable == 1){
					if (fts_wq_data->swipeup_mode_eable == 1){  
						FTS_gesture_register_d1 = 0x34;
						pr_err("[Focal][Touch] %s : open dclick and swipe mode d1= 34 \n", __func__);
					}else{
						FTS_gesture_register_d1 = 0x30;
						pr_err("[Focal][Touch] %s : open dclick and disable swipe mode d1= 30 \n", __func__);
						}
				}else if (fts_wq_data->swipeup_mode_eable == 1){
						FTS_gesture_register_d1 = 0x24;
						pr_err("[Focal][Touch] %s : open swipe mode and disable dclick mode d1= 24 \n", __func__);
				}else{
					FTS_gesture_register_d1 = 0x20;
					pr_err("[Focal][Touch] %s : only open gesture mode d1= 20 \n", __func__);
				}			
			}else{
				pr_err("[Focal][Touch] %s : disable gesture mode ", __func__);
				if (fts_wq_data->dclick_mode_eable == 1) {
					if(fts_wq_data->swipeup_mode_eable == 1){
						pr_err("[Focal][Touch] %s : open dclick and swipe mode d1= 14 \n", __func__);
						FTS_gesture_register_d1 = 0x14;
					}else{
						FTS_gesture_register_d1 = 0x10;
						pr_err("[Focal][Touch] %s : open dclick mode and disable swipe mode d1= 10 \n", __func__);
						}
				}else{
					if(fts_wq_data->swipeup_mode_eable == 1){
						pr_err("[Focal][Touch] %s : disable  dclick and open swipe mode d1= 04 \n", __func__);
						FTS_gesture_register_d1 = 0x04;
						}
					}
				}		
				for (i = 0; i < 5; i++)
   		 		{
   		 			state=fts_i2c_write_reg(i2c_client, 0xd1, FTS_gesture_register_d1);
					if (state <0){
						msleep(1);
						pr_err("[FTS][tocuh] write d1 fail %d times \n",i);
					}
       				else
            			break;
				}
				pr_err("[Focal][Touch] %s : open gesture mode d2 = %x d5 = %x d6 = %x d7 = %x \n",
					__func__, FTS_gesture_register_d2, FTS_gesture_register_d5, FTS_gesture_register_d6, FTS_gesture_register_d7);

						for (i = 0; i < 5; i++)
   		 		{
   		 			state=fts_i2c_write_reg(i2c_client, 0xd2, FTS_gesture_register_d2);
					if (state <0){
						msleep(1);
						pr_err("[FTS][tocuh] write d2 fail %d times \n",i);
					}
       				else
            			break;
				}
				for (i = 0; i < 5; i++)
   		 		{
   		 			state=fts_i2c_write_reg(i2c_client, 0xd5, FTS_gesture_register_d5);
					if (state <0){
						msleep(1);
						pr_err("[FTS][tocuh] write d5 fail %d times \n",i);
					}
       				else
            			break;
				}
				for (i = 0; i < 5; i++)
   		 		{
   		 			state=fts_i2c_write_reg(i2c_client, 0xd6, FTS_gesture_register_d6);
					if (state <0){
						msleep(1);
						pr_err("[FTS][tocuh] write d6 fail %d times \n",i);
					}
       				else
            			break;
				}
				for (i = 0; i < 5; i++)
   		 		{
   		 			state=fts_i2c_write_reg(i2c_client, 0xd7, FTS_gesture_register_d7);
					if (state <0){
						msleep(1);
						pr_err("[FTS][tocuh] write d7 fail %d times \n",i);
					}
       				else
            			break;
				}	
	/*} else {
		pr_err("[Focal][Touch] %s : Touch suspend \n", __func__);
		state = fts_i2c_write_reg(fts_wq_data->client, FTS_REG_POWER_MODE, FTS_REG_POWER_MODE_SLEEP_VALUE);
		if (state < 0)
		{
			pr_err("[FTS_err][touch]%s:Set TP to sleep mode fail, ret=%d! \n", __func__, state);
		}
	}*/
   /* for (i = 0; i < 5; i++)
    {
        fts_i2c_write_reg(i2c_client, FTS_REG_GESTURE_EN, 0x01);
        fts_i2c_write_reg(i2c_client, 0xd1, 0xff);
        fts_i2c_write_reg(i2c_client, 0xd2, 0xff);
        fts_i2c_write_reg(i2c_client, 0xd5, 0xff);
        fts_i2c_write_reg(i2c_client, 0xd6, 0xff);
        fts_i2c_write_reg(i2c_client, 0xd7, 0xff);
        fts_i2c_write_reg(i2c_client, 0xd8, 0xff);

        fts_i2c_read_reg(i2c_client, FTS_REG_GESTURE_EN, &state);
        if (state == 1)
            break;
        else
            msleep(1);
    }

    if (i >= 5)
    {
        FTS_ERROR("[GESTURE]Enter into gesture(suspend) failed!\n");
        FTS_FUNC_EXIT();
        return -1;
    }*/
    pr_err("[touch][GESTURE]Enter into gesture(suspend) successfully!\n");
    return 0;
}

/*****************************************************************************
*   Name: fts_gesture_resume
*  Brief:
*  Input:
* Output: None
* Return: None
*****************************************************************************/
int fts_gesture_resume(struct i2c_client *client)
{
    int i;
    u8 state;
    FTS_FUNC_ENTER();

    /* gesture not enable, return immediately */
    if (fts_gesture_data.mode == 0)
        return -EPERM;

    fts_gesture_data.active = 0;
	fts_gesture_data.mode = 0;
    /* Before read/write TP register, need wait TP to valid */
    fts_wait_tp_to_valid(client);
    for (i = 0; i < 5; i++)
    {
        fts_i2c_write_reg(client, FTS_REG_GESTURE_EN, 0x00);
        msleep(1);
        fts_i2c_read_reg(client, FTS_REG_GESTURE_EN, &state);
        if (state == 0)
            break;
    }

    if (i >= 5)
    {
        FTS_ERROR("[GESTURE]Clear gesture(resume) failed!\n");
    }
	pr_err("[touch][GESTURE] resume in gesture mode successfully!/n");
    return 0;
}
#endif
