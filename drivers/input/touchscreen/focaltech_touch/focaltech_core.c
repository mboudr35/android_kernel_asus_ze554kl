/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2016, FocalTech Systems, Ltd., all rights reserved.
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
* File Name: focaltech_core.c
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
* Included header files
*****************************************************************************/
#include "focaltech_core.h"
#include <linux/proc_fs.h>
#include "focaltech_flash.h"

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#define FTS_SUSPEND_LEVEL 1     /* Early-suspend level */
#endif

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define FTS_DRIVER_NAME                     "fts_ts"
#define INTERVAL_READ_REG                   1000 //interval time per read reg unit:ms
#define TIMEOUT_READ_REG                    300 //timeout of read reg unit:ms
#if FTS_POWER_SOURCE_CUST_EN
#define FTS_VTG_MIN_UV                      2600000
#define FTS_VTG_MAX_UV                      3300000
#define FTS_I2C_VTG_MIN_UV                  1800000
#define FTS_I2C_VTG_MAX_UV                  1800000
#endif
#define FTS_READ_TOUCH_BUFFER_DIVIDED       0
/*****************************************************************************
* Global variable or extern global variabls/functions
******************************************************************************/
struct i2c_client *fts_i2c_client;
struct fts_ts_data *fts_wq_data;
struct input_dev *fts_input_dev;
u8 key_record;
u8 FTS_gesture_register_d1;
u8 FTS_gesture_register_d2;
u8 FTS_gesture_register_d5;
u8 FTS_gesture_register_d6;
u8 FTS_gesture_register_d7;

//struct wake_lock ps_lock1;
extern int g_asus_lcdID;
extern void asus_psensor_disable_touch(bool enable);
extern bool g_FP_Disable_Touch ;
extern int get_audiomode(void);

unsigned char IC_FW;
u8 g_vendor_id = 0xFF;
//u8 B_VenderID;

int g_focal_touch_init_status = 0;
EXPORT_SYMBOL(g_focal_touch_init_status);

bool disable_tp_flag;
extern int HALLsensor_gpio_value(void);

EXPORT_SYMBOL(disable_tp_flag);
static bool hall_sensor_detect = false;
static bool key_already_down = false;
static bool once_key_event = false;

#if FTS_DEBUG_EN
int g_show_log = 1;
#else
int g_show_log = 0;
#endif

#if (FTS_DEBUG_EN && (FTS_DEBUG_LEVEL == 2))
char g_sz_debug[1024] = {0};
#endif

/*****************************************************************************
* Static function prototypes
*****************************************************************************/
static void fts_release_all_finger(void);
static void focal_suspend_work(void);

/*static int fts_ts_suspend(struct device *dev);
static int fts_ts_resume(struct device *dev);*/
int fts_ts_suspend(void);
int fts_ts_resume(void);
bool fts_gesture_check(void);
/* +++ asus add for print touch location +++ */
int report_touch_locatoin_count[10];
/* --- asus add for print touch location --- */
static bool touch_down_up_status;
extern bool cover_enable_touch_f;
bool key_down_status = false;
/*****************************************************************************
*  Name: fts_wait_tp_to_valid
*  Brief:   Read chip id until TP FW become valid, need call when reset/power on/resume...
*           1. Read Chip ID per INTERVAL_READ_REG(20ms)
*           2. Timeout: TIMEOUT_READ_REG(300ms)
*  Input:
*  Output:
*  Return: 0 - Get correct Device ID
*****************************************************************************/
int fts_wait_tp_to_valid(struct i2c_client *client)
{
    int ret = 0;
    int cnt = 0;
    u8  reg_value = 0;
	u8 cmd[4]={0};
	u8 val[2]={0};

    do
    {
        ret = fts_i2c_read_reg(client, FTS_REG_CHIP_ID, &reg_value);
        if ((ret < 0) || (reg_value != chip_types.chip_idh))
        {
            pr_err("[FTS_err][tocuh] TP Not Ready, ReadData = 0x%x \n", reg_value);
        }
        else if (reg_value == chip_types.chip_idh)
        {
            pr_err("[FTS][tocuh] TP Ready, Device ID = 0x%x", reg_value);
            return 0;
        }
        cnt++;
        msleep(INTERVAL_READ_REG);
    }
    while ((cnt * 20) < TIMEOUT_READ_REG);

    /* error: not get correct reg data */
	cmd[0]=0x55;
	cmd[1]=0xaa;
	fts_i2c_write(client, cmd,2);
	cmd[0]=0x90;
	cmd[1]=cmd[2]=cmd[3]=0x00;
	fts_i2c_read(client, cmd,4,val,2);
	pr_err("[FTS][Touch]tp id =0x%x%x \n",val[0],val[1]);
	if(val[0]== chip_types.chip_idh)
		return 0;
    return -1;
}

/*****************************************************************************
*  Name: fts_recover_state
*  Brief: Need execute this function when reset
*  Input:
*  Output:
*  Return:
*****************************************************************************/
void fts_tp_state_recovery(struct i2c_client *client)
{
    /* wait tp stable */
    fts_wait_tp_to_valid(client);
    /* recover TP charger state 0x8B */
    /* recover TP glove state 0xC0 */
    /* recover TP cover state 0xC1 */
    fts_ex_mode_recovery(client);

#if FTS_PSENSOR_EN
    fts_proximity_recovery(client);
#endif

    /* recover TP gesture state 0xD0 */
/*#if FTS_GESTURE_EN
    fts_gesture_recovery(client);
#endif*/

    fts_release_all_finger();
}


/*****************************************************************************
*  Name: fts_reset_proc
*  Brief: Execute reset operation
*  Input: hdelayms - delay time unit:ms
*  Output:
*  Return:
*****************************************************************************/
int fts_reset_proc(int hdelayms)
{
	pr_err("[FTS][touch] %s ,reset touch !\n",__func__);
    gpio_direction_output(fts_wq_data->pdata->reset_gpio, 0);
    msleep(10);
    gpio_direction_output(fts_wq_data->pdata->reset_gpio, 1);
    msleep(hdelayms);

    return 0;
}
void ftxxxx_reset_tp(int HighOrLow)
{
	pr_info("[Focal][Touch] %s : set tp reset pin to %d\n", __func__, HighOrLow);
	if (HighOrLow)
		gpio_direction_output(fts_wq_data->pdata->reset_gpio, 1);
	else 
		gpio_direction_output(fts_wq_data->pdata->reset_gpio, 0);
}
void ftxxxx_disable_touch(bool flag)
{
	if (g_focal_touch_init_status == 1){
		if (flag) {
			disable_tp_flag = true;
			fts_release_all_finger();
			printk("[Focal][Touch] %s: proximity trigger disable touch !\n", __func__);
		} else {
			disable_tp_flag = false;
			printk("[Focal][Touch] %s: proximity trigger enable touch  !\n", __func__);
		}
	}else{
		asus_psensor_disable_touch(flag);
	}
	
}
EXPORT_SYMBOL(ftxxxx_disable_touch);


/*****************************************************************************
* Power Control
*****************************************************************************/
#if FTS_POWER_SOURCE_CUST_EN
static int fts_power_source_init(struct fts_ts_data *data)
{
    int rc;

    FTS_FUNC_ENTER();

    data->vdd = regulator_get(&data->client->dev, "vdd");
    if (IS_ERR(data->vdd))
    {
        rc = PTR_ERR(data->vdd);
        FTS_ERROR("Regulator get failed vdd rc=%d", rc);
    }

    if (regulator_count_voltages(data->vdd) > 0)
    {
        rc = regulator_set_voltage(data->vdd, FTS_VTG_MIN_UV, FTS_VTG_MAX_UV);
        if (rc)
        {
            FTS_ERROR("Regulator set_vtg failed vdd rc=%d", rc);
            goto reg_vdd_put;
        }
    }

    data->vcc_i2c = regulator_get(&data->client->dev, "vcc_i2c");
    if (IS_ERR(data->vcc_i2c))
    {
        rc = PTR_ERR(data->vcc_i2c);
        FTS_ERROR("Regulator get failed vcc_i2c rc=%d", rc);
        goto reg_vdd_set_vtg;
    }

    if (regulator_count_voltages(data->vcc_i2c) > 0)
    {
        rc = regulator_set_voltage(data->vcc_i2c, FTS_I2C_VTG_MIN_UV, FTS_I2C_VTG_MAX_UV);
        if (rc)
        {
            FTS_ERROR("Regulator set_vtg failed vcc_i2c rc=%d", rc);
            goto reg_vcc_i2c_put;
        }
    }

    FTS_FUNC_EXIT();
    return 0;

reg_vcc_i2c_put:
    regulator_put(data->vcc_i2c);
reg_vdd_set_vtg:
    if (regulator_count_voltages(data->vdd) > 0)
        regulator_set_voltage(data->vdd, 0, FTS_VTG_MAX_UV);
reg_vdd_put:
    regulator_put(data->vdd);
    FTS_FUNC_EXIT();
    return rc;
}

static int fts_power_source_ctrl(struct fts_ts_data *data, int enable)
{
    int rc;

    FTS_FUNC_ENTER();
    if (enable)
    {
        rc = regulator_enable(data->vdd);
        if (rc)
        {
            FTS_ERROR("Regulator vdd enable failed rc=%d", rc);
        }

        rc = regulator_enable(data->vcc_i2c);
        if (rc)
        {
            FTS_ERROR("Regulator vcc_i2c enable failed rc=%d", rc);
        }
    }
    else
    {
        rc = regulator_disable(data->vdd);
        if (rc)
        {
            FTS_ERROR("Regulator vdd disable failed rc=%d", rc);
        }
        rc = regulator_disable(data->vcc_i2c);
        if (rc)
        {
            FTS_ERROR("Regulator vcc_i2c disable failed rc=%d", rc);
        }
    }
    FTS_FUNC_EXIT();
    return 0;
}

#endif


/*****************************************************************************
*  Reprot related
*****************************************************************************/
/*****************************************************************************
*  Name: fts_release_all_finger
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void fts_release_all_finger(void)
{
	int i ;
#if FTS_MT_PROTOCOL_B_EN
    unsigned int finger_count=0;
#endif

    mutex_lock(&fts_wq_data->report_mutex);
#if FTS_MT_PROTOCOL_B_EN
    for (finger_count = 0; finger_count < FTS_MAX_POINTS; finger_count++)
    {
        input_mt_slot(fts_input_dev, finger_count);
        input_mt_report_slot_state(fts_input_dev, MT_TOOL_FINGER, false);
    }
#else
    input_mt_sync(fts_input_dev);
#endif
	 if (fts_wq_data->pdata->have_key)
    {
		for (i = 0; i < fts_wq_data->pdata->key_number; i++)
		{
			input_report_key(fts_input_dev, fts_wq_data->pdata->keys[i], 0);
		}
	 }
    input_report_key(fts_input_dev, BTN_TOUCH, 0);
    input_sync(fts_input_dev);
    mutex_unlock(&fts_wq_data->report_mutex);
}

/*****************************************************************************
*  Name: fts_read_touchdata
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_read_touchdata(struct fts_ts_data *data)
{
    u8 buf[POINT_READ_BUF] = { 0 };
    u8 pointid = FTS_MAX_ID;
    int ret = -1;
    int i;
	u8 state;
    struct ts_event * event = &(data->event);

#if FTS_GESTURE_EN
       if (data->suspended)
       {
           if ((fts_wq_data->dclick_mode_eable == 1) | (fts_wq_data->swipeup_mode_eable == 1) | (fts_wq_data->gesture_mode_eable == 1)) {
           		i2c_smbus_read_i2c_block_data(fts_wq_data->client, 0xd0, 1, &state);
				if (state==1){
            		pr_err("[fts][touch] read gesture id ! \n ");
                	fts_gesture_readdata(data->client);
					}
                return 1;
            }
        }
#endif

#if FTS_PSENSOR_EN
    if ( (fts_sensor_read_data(data) != 0) && (data->suspended == 1) )
    {
        return 1;
    }
#endif
#if FTS_READ_TOUCH_BUFFER_DIVIDED
    memset(buf, 0xFF, POINT_READ_BUF);
    memset(event, 0, sizeof(struct ts_event));

    buf[0] = 0x00;
    ret = fts_i2c_read(data->client, buf, 1, buf, 3);
    if (ret < 0)
    {
        FTS_ERROR("%s read touchdata failed.", __func__);
        return ret;
    }
    event->touch_point = 0;
    event->point_num=buf[FTS_TOUCH_POINT_NUM] & 0x0F;
    if (event->point_num > FTS_MAX_POINTS)
        event->point_num = FTS_MAX_POINTS;
    buf[3] = 0x03;
    if (event->point_num > 0)
    {
        fts_i2c_read(data->client, buf+3, 1, buf+3, event->point_num * FTS_ONE_TCH_LEN);
    }
#else
    ret = fts_i2c_read(data->client, buf, 1, buf, POINT_READ_BUF);
    if (ret < 0)
    {
        FTS_ERROR("[B]Read touchdata failed, ret: %d", ret);
        return ret;
    }
    memset(event, 0, sizeof(struct ts_event));
    event->point_num = buf[FTS_TOUCH_POINT_NUM] & 0x0F;
    if (event->point_num > FTS_MAX_POINTS)
        event->point_num = FTS_MAX_POINTS;
    event->touch_point = 0;
#endif

#if (FTS_DEBUG_EN && (FTS_DEBUG_LEVEL == 2))
    {
        int len = event->point_num * FTS_ONE_TCH_LEN;
        int count = 0;
        memset(g_sz_debug, 0, 1024);
        if (len > (POINT_READ_BUF-3))
        {
            len = POINT_READ_BUF-3;
        }
        else if (len == 0)
        {
            len += FTS_ONE_TCH_LEN;
        }
        count += sprintf(g_sz_debug, "%02X,%02X,%02X", buf[0], buf[1], buf[2]);
        for (i = 0; i < len; i++)
        {
            count += sprintf(g_sz_debug+count, ",%02X", buf[i+3]);
        }
        FTS_DEBUG("buffer: %s", g_sz_debug);
    }
#endif

    for (i = 0; i < FTS_MAX_POINTS; i++)
    {
        pointid = (buf[FTS_TOUCH_ID_POS + FTS_ONE_TCH_LEN * i]) >> 4;
        if (pointid >= FTS_MAX_ID)
            break;
        else
            event->touch_point++;

        event->au16_x[i] =
            (s16) (buf[FTS_TOUCH_X_H_POS + FTS_ONE_TCH_LEN * i] & 0x0F) <<
            8 | (s16) buf[FTS_TOUCH_X_L_POS + FTS_ONE_TCH_LEN * i];
        event->au16_y[i] =
            (s16) (buf[FTS_TOUCH_Y_H_POS + FTS_ONE_TCH_LEN * i] & 0x0F) <<
            8 | (s16) buf[FTS_TOUCH_Y_L_POS + FTS_ONE_TCH_LEN * i];
        event->au8_touch_event[i] =
            buf[FTS_TOUCH_EVENT_POS + FTS_ONE_TCH_LEN * i] >> 6;
        event->au8_finger_id[i] =
            (buf[FTS_TOUCH_ID_POS + FTS_ONE_TCH_LEN * i]) >> 4;
        event->area[i] =
            (buf[FTS_TOUCH_AREA_POS + FTS_ONE_TCH_LEN * i]) >> 4;
        event->pressure[i] =
            (s16) buf[FTS_TOUCH_PRE_POS + FTS_ONE_TCH_LEN * i];

        if (0 == event->area[i])
            event->area[i] = 0x09;

        if (0 == event->pressure[i])
            event->pressure[i] = 0x3f;

        if ((event->au8_touch_event[i]==0 || event->au8_touch_event[i]==2)&&(event->point_num==0))
        {
            return -1;
        }
    }
    
    if(event->touch_point == 0)
    {
        return -1;
    }
    
    return 0;
}

#if FTS_TEST_EN
extern int fts_test_vk(u16 x, u16 y);
extern int visual_key_active;
#endif

static int fts_input_dev_report_key_event(struct ts_event *event, struct fts_ts_data *data)
{
    int i;
#if FTS_TEST_EN
    if( 1 == visual_key_active)
    {
    	 fts_test_vk(event->au16_x[0], event->au16_y[0]);
		 return 0;
    }
#endif
    if (data->pdata->have_key)
    {
        if ( (1 == event->touch_point || 1 == event->point_num) &&
             (event->au16_y[0] == data->pdata->key_y_coord))
        {
			if (g_FP_Disable_Touch == 1){
				printk("[focal][touch] fp disable touch \n");
				for (i = 0; i < data->pdata->key_number; i++)
                {
                    input_report_key(data->input_dev, data->pdata->keys[i], 0);
					once_key_event = false;
                }
				input_sync(data->input_dev);
				return 0;
			}
            if (event->point_num == 0)
            {
                FTS_DEBUG("Keys All Up!");
                for (i = 0; i < data->pdata->key_number; i++)
                {
					if (hall_sensor_detect)
						msleep(20);
					if ((HALLsensor_gpio_value() > 0) || (key_already_down == true))
					{
						input_report_key(data->input_dev, data->pdata->keys[i], 0);
						once_key_event = false;
						printk("[focal][touch] send keycode = %d All UP\n", data->pdata->keys[i]);
						if (i == (data->pdata->key_number - 1))
							key_already_down = false;
					}
					else
					{
						hall_sensor_detect = true;
						printk("[focal][touch] disable VK due to hall sensor CLOSE\n");
					}
                }
            }
            else
            {
                for (i = 0; i < data->pdata->key_number; i++)
                {
                    if (event->au16_x[0] > (data->pdata->key_x_coords[i] - FTS_KEY_WIDTH) &&
                        event->au16_x[0] < (data->pdata->key_x_coords[i] + FTS_KEY_WIDTH))
                    {

                        if (event->au8_touch_event[i]== 0 ||
                            event->au8_touch_event[i] == 2)
                        {
							if (hall_sensor_detect)
								msleep(20);
							if (HALLsensor_gpio_value() > 0)
							{
								if(once_key_event == false){
								input_report_key(data->input_dev, data->pdata->keys[i], 1);
								key_already_down = true;
								once_key_event = true;
								FTS_DEBUG("Key%d(%d, %d) DOWN!", i, event->au16_x[0], event->au16_y[0]);
								printk("[focal][touch] send keycode = %d DOWN\n", data->pdata->keys[i]);
								}
							}
							else
							{
								hall_sensor_detect = true;
								printk("[focal][touch] disable VK due to hall sensor CLOSE\n");
							}
                        }
                        else
                        {
							if (hall_sensor_detect)
								msleep(20);
							if ((HALLsensor_gpio_value() > 0) || (key_already_down == true))
							{
								input_report_key(data->input_dev, data->pdata->keys[i], 0);
								FTS_DEBUG("Key%d(%d, %d) Up!", i, event->au16_x[0], event->au16_y[0]);
								printk("[focal][touch] send keycode = %d UP\n", data->pdata->keys[i]);
							    key_already_down = false;
								once_key_event = false;
							}
							else
							{
								hall_sensor_detect = true;
								printk("[focal][touch] disable VK due to hall sensor CLOSE\n");
							}
						}
                        break;
                    }
                }
            }
            input_sync(data->input_dev);
            return 0;
        }
    }

    return -1;
}



/*#if FTS_TEST_EN
extern int fts_test_vk(u16 x, u16 y);
#endif
static int fts_input_dev_report_key_event(struct ts_event *event, struct fts_ts_data *data)
{
    int i;
#if FTS_TEST_EN
    fts_test_vk(event->au16_x[0], event->au16_y[0]);
#endif
    if (data->pdata->have_key)
    {
        if ( (1 == event->touch_point || 1 == event->point_num) &&
             (event->au16_y[0] == data->pdata->key_y_coord))
        {

            if (event->point_num == 0)
            {
                FTS_DEBUG("Keys All Up!");
                for (i = 0; i < data->pdata->key_number; i++)
                {
                    input_report_key(data->input_dev, data->pdata->keys[i], 0);
                }
            }
            else
            {
                for (i = 0; i < data->pdata->key_number; i++)
                {
                    if (event->au16_x[0] > (data->pdata->key_x_coords[i] - FTS_KEY_WIDTH) &&
                        event->au16_x[0] < (data->pdata->key_x_coords[i] + FTS_KEY_WIDTH))
                    {

                        if (event->au8_touch_event[i]== 0 ||
                            event->au8_touch_event[i] == 2)
                        {
                            input_report_key(data->input_dev, data->pdata->keys[i], 1);
                            FTS_DEBUG("Key%d(%d, %d) DOWN!", i, event->au16_x[0], event->au16_y[0]);
                        }
                        else
                        {
                            input_report_key(data->input_dev, data->pdata->keys[i], 0);
                            FTS_DEBUG("Key%d(%d, %d) Up!", i, event->au16_x[0], event->au16_y[0]);
                        }
                        break;
                    }
                }
            }
            input_sync(data->input_dev);
            return 0;
        }
    }

    return -1;
}*/

#if FTS_MT_PROTOCOL_B_EN
static int fts_input_dev_report_b(struct ts_event *event, struct fts_ts_data *data)
{
    int i = 0;
    int uppoint = 0;
    int touchs = 0;
    for (i = 0; i < event->touch_point; i++)
    {
        if (event->au8_finger_id[i] >= data->pdata->max_touch_number)
        {
            break;
        }
        input_mt_slot(data->input_dev, event->au8_finger_id[i]);

        if (event->au8_touch_event[i] == FTS_TOUCH_DOWN || event->au8_touch_event[i] == FTS_TOUCH_CONTACT)
        {
            input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, true);

#if FTS_REPORT_PRESSURE_EN
#if FTS_FORCE_TOUCH_EN
            if (event->pressure[i] <= 0)
            {
                FTS_ERROR("[B]Illegal pressure: %d", event->pressure[i]);
                event->pressure[i] = 1;
            }
#else
            event->pressure[i] = 0x3f;
#endif
            input_report_abs(data->input_dev, ABS_MT_PRESSURE, event->pressure[i]);
#endif

            if (event->area[i] <= 0)
            {
                FTS_ERROR("[B]Illegal touch-major: %d", event->area[i]);
                event->area[i] = 1;
            }
            input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->area[i]);

            input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->au16_x[i]);
            input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->au16_y[i]);
            touchs |= BIT(event->au8_finger_id[i]);
            data->touchs |= BIT(event->au8_finger_id[i]);
		/* +++ asus add for print touch location +++ */
			if ((report_touch_locatoin_count[i] % 200) == 0) {
				printk("[Focal][Touch] report id=%d event=%d x=%d y=%d pressure=%d area=%d\n", event->au8_finger_id[i],
				event->au8_touch_event[i], event->au16_x[i], event->au16_y[i], event->pressure[i], event->area[i]);
				report_touch_locatoin_count[i] = 1;
			}
		report_touch_locatoin_count[i] += 1;
			/* --- asus jacob add for print touch location --- */
#if FTS_REPORT_PRESSURE_EN
            FTS_DEBUG("[B]P%d(%d, %d)[p:%d,tm:%d] DOWN!", event->au8_finger_id[i], event->au16_x[i],
                      event->au16_y[i], event->pressure[i], event->area[i]);
#else
            FTS_DEBUG("[B]P%d(%d, %d)[tm:%d] DOWN!", event->au8_finger_id[i], event->au16_x[i], event->au16_y[i], event->area[i]);
#endif
        }
        else
        {
            uppoint++;
			/* +++ asus add for print touch location +++ */
			report_touch_locatoin_count[i] = 0;
			/* --- asus add for print touch location --- */
            input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
#if FTS_REPORT_PRESSURE_EN
            input_report_abs(data->input_dev, ABS_MT_PRESSURE, 0);
#endif
            data->touchs &= ~BIT(event->au8_finger_id[i]);
            FTS_DEBUG("[B]P%d UP!", event->au8_finger_id[i]);
        }
    }

    if (unlikely(data->touchs ^ touchs))
    {
        for (i = 0; i < data->pdata->max_touch_number; i++)
        {
            if (BIT(i) & (data->touchs ^ touchs))
            {
                FTS_DEBUG("[B]P%d UP!", i);
                input_mt_slot(data->input_dev, i);
                input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
#if FTS_REPORT_PRESSURE_EN
                input_report_abs(data->input_dev, ABS_MT_PRESSURE, 0);
#endif
            }
        }
    }

    data->touchs = touchs;
    if (event->touch_point == uppoint)
    {
        FTS_DEBUG("Points All Up!");
		if (touch_down_up_status == 1) {
			printk("[Focal][Touch] touch up !\n");
			touch_down_up_status = 0;
		}
		/* +++ asus jacob add for print touch location +++ */
		memset(report_touch_locatoin_count, 0, sizeof(report_touch_locatoin_count));
		/* --- asus jacob add for print touch location --- */
        input_report_key(data->input_dev, BTN_TOUCH, 0);
		key_down_status = false;
    }
    else
    {
    	if (touch_down_up_status == 0) {
			touch_down_up_status = 1;
			printk("[Focal][Touch] touch down !\n");
			printk("[Focal][Touch] id=%d event=%d x=%d y=%d pressure=%d area=%d\n", event->au8_finger_id[0],
			event->au8_touch_event[0], event->au16_x[0], event->au16_y[0], event->pressure[0], event->area[0]);
		}
        input_report_key(data->input_dev, BTN_TOUCH, event->touch_point > 0);
		key_down_status = true;
    }

    input_sync(data->input_dev);

    return 0;

}

#else
static int fts_input_dev_report_a(struct ts_event *event,struct fts_ts_data *data)
{
    int i =0;
    int uppoint = 0;
    int touchs = 0;

    for (i = 0; i < event->touch_point; i++)
    {

        if (event->au8_touch_event[i] == FTS_TOUCH_DOWN || event->au8_touch_event[i] == FTS_TOUCH_CONTACT)
        {
            input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->au8_finger_id[i]);
#if FTS_REPORT_PRESSURE_EN
#if FTS_FORCE_TOUCH_EN
            if (event->pressure[i] <= 0)
            {
                FTS_ERROR("[B]Illegal pressure: %d", event->pressure[i]);
                event->pressure[i] = 1;
            }
#else
            event->pressure[i] = 0x3f;
#endif
            input_report_abs(data->input_dev, ABS_MT_PRESSURE, event->pressure[i]);
#endif

            if (event->area[i] <= 0)
            {
                FTS_ERROR("[B]Illegal touch-major: %d", event->area[i]);
                event->area[i] = 1;
            }
            input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->area[i]);

            input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->au16_x[i]);
            input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->au16_y[i]);
			/* +++ asus add for print touch location +++ */
			if ((report_touch_locatoin_count[i] % 200) == 0) {
				printk("[Focal][Touch] report id=%d event=%d x=%d y=%d pressure=%d area=%d\n", event->au8_finger_id[i],
				event->au8_touch_event[i], event->au16_x[i], event->au16_y[i], event->pressure[i], event->area[i]);
				report_touch_locatoin_count[i] = 1;
			}
			report_touch_locatoin_count[i] += 1;
			/* --- asus jacob add for print touch location --- */
            input_mt_sync(data->input_dev);

#if FTS_REPORT_PRESSURE_EN
            FTS_DEBUG("[B]P%d(%d, %d)[p:%d,tm:%d] DOWN!", event->au8_finger_id[i], event->au16_x[i],
                      event->au16_y[i], event->pressure[i], event->area[i]);
#else
            FTS_DEBUG("[B]P%d(%d, %d)[tm:%d] DOWN!", event->au8_finger_id[i], event->au16_x[i], event->au16_y[i], event->area[i]);
#endif
        }
        else
        {
            uppoint++;
			/* +++ asus add for print touch location +++ */
			report_touch_locatoin_count[i] = 0;
			/* --- asus add for print touch location --- */
        }
    }

    data->touchs = touchs;
    if (event->touch_point == uppoint)
    {
       // FTS_DEBUG("Points All Up!");
       if (touch_down_up_status == 1) {
       		touch_down_up_status = 0;
	   		printk("[Focal][Touch] touch up !\n");
       	}
		/* +++ asus jacob add for print touch location +++ */
		memset(report_touch_locatoin_count, 0, sizeof(report_touch_locatoin_count));
		/* --- asus jacob add for print touch location --- */
        input_report_key(data->input_dev, BTN_TOUCH, 0);
        input_mt_sync(data->input_dev);
    }
    else
    {
        input_report_key(data->input_dev, BTN_TOUCH, event->touch_point > 0);
		if (touch_down_up_status == 0) {
			touch_down_up_status = 1;
			printk("[Focal][Touch] touch down !\n");
			printk("[Focal][Touch] id=%d event=%d x=%d y=%d pressure=%d area=%d\n", event->au8_finger_id[0],
			event->au8_touch_event[0], event->au16_x[0], event->au16_y[0], event->pressure[0], event->area[0]);
		}
    }

    input_sync(data->input_dev);

    return 0;
}
#endif



/*****************************************************************************
*  Name: fts_report_value
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void fts_report_value(struct fts_ts_data *data)
{
    struct ts_event *event = &data->event;
   /* int i;
    int uppoint = 0;
    int touchs = 0;
    int pressures=0;*/

    if (0 == fts_input_dev_report_key_event(event, data))
    {
        return;
    }
#if FTS_MT_PROTOCOL_B_EN
    fts_input_dev_report_b(event, data);
#else
    fts_input_dev_report_a(event, data);
#endif


    return;

    /*FTS_DEBUG("point number: %d, touch point: %d", event->point_num,
              event->touch_point);*/

/*    if (data->pdata->have_key)
    {
        if ( (1 == event->touch_point || 1 == event->point_num) &&
             (event->au16_y[0] == data->pdata->key_y_coord))
        {

            if (event->point_num == 0)
            {
                FTS_DEBUG("Keys All Up!");
			if (key_record) {
				if (key_record & 0x1) {
					input_report_key(data->input_dev, KEY_BACK, 0);
					printk("[touch]Key_back%d(%d, %d) Up!", i, event->au16_x[0], event->au16_y[0]);
				}
				if (key_record & 0x2) {
					input_report_key(data->input_dev, KEY_HOME, 0);
					printk("[touch]Key_home%d(%d, %d) Up!", i, event->au16_x[0], event->au16_y[0]);
				}
				if (key_record & 0x4) {
					input_report_key(data->input_dev, KEY_MENU, 0);
					printk("[touch]Key_menu%d(%d, %d) Up!", i, event->au16_x[0], event->au16_y[0]);
				}
			}
			key_record = 0;*/
      /*          for (i = 0; i < 3; i++)
                {
                	input_report_key(data->input_dev, data->pdata->keys[i], 0);
                }*/
         /*  }
            else
            {
            	printk("[touch]Keys y=%d x=%d \n",event->au16_y[0], event->au16_x[0]);
				for (i = 0; i < 3; i++)
                {
				if((event->au16_x[i] > 150 )&(event->au16_x[i] < 250))
                    {
                        if (event->au8_touch_event[i]== 0 ||
                            event->au8_touch_event[i] == 2)
                        {
                            input_report_key(data->input_dev, KEY_BACK, 1);
							key_record |= 1;
                            printk("[touch]Key_back%d(%d, %d) DOWN!", i, event->au16_x[0], event->au16_y[0]);
                        }
                        else
                        {
                        key_record &= 0xfe;
                            input_report_key(data->input_dev, KEY_BACK, 0);
                            printk("[touch]Key_back%d(%d, %d) Up!", i, event->au16_x[0], event->au16_y[0]);
                        }

					}else if((event->au16_x[i] > 550 )&(event->au16_x[i] < 650))
						{
						if (event->au8_touch_event[i]== 0 ||
                            event->au8_touch_event[i] == 2)
                        {
                            input_report_key(data->input_dev, KEY_MENU, 1);
							key_record |= 1 << 2;
                            printk("[touch]Key_menu%d(%d, %d) DOWN!", i, event->au16_x[0], event->au16_y[0]);
                        }
                        else
                        {
                            input_report_key(data->input_dev, KEY_MENU, 0);
							key_record &= 0xfb;
                           printk("[ouch]Key_menu%d(%d, %d) Up!", i, event->au16_x[0], event->au16_y[0]);
                        }
					}
				}*/
             /*   for (i = 0; i < data->pdata->key_number; i++)
                {
                    if (event->au16_x[0] > (data->pdata->key_x_coords[i] - FTS_KEY_WIDTH) &&
                        event->au16_x[0] < (data->pdata->key_x_coords[i] + FTS_KEY_WIDTH))
                    {

                        if (event->au8_touch_event[i]== 0 ||
                            event->au8_touch_event[i] == 2)
                        {
                            input_report_key(data->input_dev, KEY_BACK, 1);
                            FTS_DEBUG("Key%d(%d, %d) DOWN!", i, event->au16_x[0], event->au16_y[0]);
                        }
                        else
                        {
                            input_report_key(data->input_dev, data->pdata->keys[i], 0);
                            FTS_DEBUG("Key%d(%d, %d) Up!", i, event->au16_x[0], event->au16_y[0]);
                        }
                        break;
                    }
                }*/
           /* }
            input_sync(data->input_dev);
            return;
        }
    }*/
/*#if FTS_MT_PROTOCOL_B_EN
    for (i = 0; i < event->touch_point; i++)
    {
        input_mt_slot(data->input_dev, event->au8_finger_id[i]);

        if (event->au8_touch_event[i] == FTS_TOUCH_DOWN || event->au8_touch_event[i] == FTS_TOUCH_CONTACT)
        {
            input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, true);

            if (FTS_REPORT_PRESSURE_EN)
            {
                if (FTS_FORCE_TOUCH_EN)
                {
                    if (event->pressure[i] > 0)
                    {
                        pressures = event->pressure[i];
                    }
                    else
                    {
                        FTS_ERROR("[B]Illegal pressure: %d", event->pressure[i]);
                        pressures = 1;
                    }
                }
                else
                {
                    pressures = 0x3f;
                }

                input_report_abs(data->input_dev, ABS_MT_PRESSURE, pressures);
            }

            if (event->area[i] > 0)
            {
                input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->area[i]);//0x05
            }
            else
            {
                FTS_ERROR("[B]Illegal touch-major: %d", event->area[i]);
                input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 1);//0x05
            }

            input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->au16_x[i]);
            input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->au16_y[i]);
            touchs |= BIT(event->au8_finger_id[i]);
            data->touchs |= BIT(event->au8_finger_id[i]);

            if (FTS_REPORT_PRESSURE_EN)
            {
                FTS_DEBUG("[B]P%d(%d, %d)[p:%d,tm:%d] DOWN!", event->au8_finger_id[i], event->au16_x[i], event->au16_y[i],
                          pressures, event->area[i]);
            }
            else
            {
                FTS_DEBUG("[B]P%d(%d, %d)[tm:%d] DOWN!", event->au8_finger_id[i], event->au16_x[i], event->au16_y[i], event->area[i]);
            }
        }
        else
        {
            uppoint++;
            input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
            data->touchs &= ~BIT(event->au8_finger_id[i]);
            FTS_DEBUG("[B]P%d UP!", event->au8_finger_id[i]);
        }
    }

    if (unlikely(data->touchs ^ touchs))
    {
        for (i = 0; i < data->pdata->max_touch_number; i++)
        {
            if (BIT(i) & (data->touchs ^ touchs))
            {
                FTS_DEBUG("[B]P%d UP!", i);
                input_mt_slot(data->input_dev, i);
                input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
            }
        }
    }
#else
    for (i = 0; i < event->touch_point; i++)
    {

        if (event->au8_touch_event[i] == FTS_TOUCH_DOWN || event->au8_touch_event[i] == FTS_TOUCH_CONTACT)
        {
            input_report_key(data->input_dev, BTN_TOUCH, 1);
            input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, event->au8_finger_id[i]);
            if (FTS_REPORT_PRESSURE_EN)
            {
                if (FTS_FORCE_TOUCH_EN)
                {
                    if (event->pressure[i] > 0)
                    {
                        pressures = event->pressure[i];
                    }
                    else
                    {
                        FTS_ERROR("[A]Illegal pressure: %d", event->pressure[i]);
                        pressures = 1;
                    }
                }
                else
                {
                    pressures = 0x3f;
                }

                input_report_abs(data->input_dev, ABS_MT_PRESSURE, pressures);
            }

            if (event->area[i] > 0)
            {
                input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, event->area[i]);//0x05
            }
            else
            {
                FTS_ERROR("[A]Illegal touch-major: %d", event->area[i]);
                input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 1);//0x05
            }
            input_report_abs(data->input_dev, ABS_MT_POSITION_X, event->au16_x[i]);
            input_report_abs(data->input_dev, ABS_MT_POSITION_Y, event->au16_y[i]);

            input_mt_sync(data->input_dev);


            if (FTS_REPORT_PRESSURE_EN)
            {
                FTS_DEBUG("[A]P%d(%d, %d)[p:%d,tm:%d] DOWN!", event->au8_finger_id[i], event->au16_x[i], event->au16_y[i],
                          pressures, event->area[i]);
            }
            else
            {
                FTS_DEBUG("[A]P%d(%d, %d)[tm:%d] DOWN!", event->au8_finger_id[i], event->au16_x[i], event->au16_y[i], event->area[i]);
            }
        }
        else
        {
            uppoint++;
        }
    }
#endif

    data->touchs = touchs;
    if (event->touch_point == uppoint)
    {
        FTS_DEBUG("Points All Up!");
        input_report_key(data->input_dev, BTN_TOUCH, 0);
    }
    else
    {
        input_report_key(data->input_dev, BTN_TOUCH, event->touch_point > 0);
    }

    input_sync(data->input_dev);*/
}
static void focal_suspend_work(void)
{	
//		struct fts_ts_data *data = dev_get_drvdata(dev);
		int retval = 0;
		printk("[FTS][touch]%s:focal touch suspend \n", __func__);
		if (fts_wq_data->suspended)
		{
			printk("[FTS][touch]%s: Already in suspend state \n", __func__);
			printk("[FTS][touch]%s: exit \n", __func__);
			return ;
		}
	
//		fts_release_all_finger();
#if FTS_GESTURE_EN
		if ((fts_wq_data->dclick_mode_eable == 1) | (fts_wq_data->swipeup_mode_eable == 1) | (fts_wq_data->gesture_mode_eable == 1)) {
			fts_release_all_finger();
			retval = fts_gesture_suspend(fts_wq_data->client);
			if (retval == 0)
			{
				/* Enter into gesture mode(suspend) */
				retval = enable_irq_wake(fts_wq_data->client->irq);
				if (retval)
					printk("[FTS][touch]%s: set_irq_wake failed \n", __func__);
				fts_wq_data->suspended = true;
				printk("[FTS][touch]%s: exit \n", __func__);
            	//fts_release_all_finger();
				return ;
			}
		}
#endif
	/* +++ asus add for print touch location +++ */
	memset(report_touch_locatoin_count, 0, sizeof(report_touch_locatoin_count));
	/* --- asus add for print touch location --- */
#if FTS_PSENSOR_EN
		if ( fts_sensor_suspend(fts_wq_data) != 0 )
		{
			enable_irq_wake(fts_wq_data->client->irq);
			fts_wq_data->suspended = true;
			return ;
		}
#endif
	
#if FTS_ESDCHECK_EN
		fts_esdcheck_suspend();
#endif
	
		disable_irq_nosync(fts_wq_data->client->irq);
		fts_release_all_finger();
		/* TP enter sleep mode */
		//fts_i2c_write_reg(fts_wq_data->client, 0xd0, 0x00);
		retval = fts_i2c_write_reg(fts_wq_data->client, FTS_REG_POWER_MODE, FTS_REG_POWER_MODE_SLEEP_VALUE);
		if (retval < 0)
		{
			printk("[FTS_err][touch]%s:Set TP to sleep mode fail, ret=%d! \n", __func__, retval);
		}
		fts_wq_data->suspended = true;
		printk("[FTS][touch]%s: exit \n", __func__);
	    //fts_release_all_finger();
		touch_down_up_status = 0;
		return ;
}

static void focal_resume_work(struct work_struct *work)
{		
	//	struct fts_ts_data *data = dev_get_drvdata(dev);
		int err;
		char val;
		printk("[FTS][touch]%s: Enter\n", __func__);
		if (!fts_wq_data->suspended)
		{
			printk("[FTS][touch] %s:Already in awake state exit resume function", __func__);
			return ;
		}
		//mutex_lock(&fts_wq_data->report_mutex);
		//disable_irq_nosync(fts_wq_data->client->irq);
#if FTS_GESTURE_EN
		if (fts_gesture_resume(fts_wq_data->client) == 0)
		{
			err = disable_irq_wake(fts_wq_data->client->irq);
			if (err)
				printk("[FTS_err][touch]%s: disable_irq_wake failed\n", __func__);
			disable_irq_nosync(fts_wq_data->client->irq);
			fts_reset_proc(300);
			fts_tp_state_recovery(fts_wq_data->client);
			fts_wq_data->suspended = false;
			enable_irq(fts_wq_data->client->irq);
			//mutex_unlock(&fts_wq_data->report_mutex);
			printk("[FTS][touch]%s: exit \n", __func__);
			return ;
		}
#endif
	
//#if (!FTS_CHIP_IDC)
		fts_reset_proc(300);
//#endif
	
		fts_tp_state_recovery(fts_wq_data->client);
	
#if FTS_PSENSOR_EN
		if ( fts_sensor_resume(fts_wq_data) != 0 )
		{
			disable_irq_wake(fts_wq_data->client->irq);
			fts_wq_data->suspended = false;
			printk("[FTS][touch]%s: exit \n", __func__);
			return ;
		}
#endif
	
		err = fts_i2c_read_reg(fts_i2c_client, 0xA6, &val);
		printk("[FTS][Touch] %s:read 0xA6: %02X, ret: %d", __func__, val, err);
	
		fts_wq_data->suspended = false;
	//mutex_unlock(&fts_wq_data->report_mutex);
		enable_irq(fts_wq_data->client->irq);
	
#if FTS_ESDCHECK_EN
		fts_esdcheck_resume();
#endif
		printk("[FTS][touch]%s: exit \n", __func__);
		return ;
}

#if 0

/*****************************************************************************
*  Name: fts_touch_irq_work
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void fts_touch_irq_work(struct work_struct *work)
{
    int ret = -1;

#if FTS_ESDCHECK_EN
    fts_esdcheck_set_intr(1);
#endif
	//disable_irq_nosync(fts_wq_data->client->irq);
	mutex_lock(&fts_wq_data->report_mutex);
	if(g_focal_touch_init_status == 1) {
   		ret = fts_read_touchdata(fts_wq_data);
    	if (ret == 0 && (!disable_tp_flag))
	   	{
            
	        fts_report_value(fts_wq_data);
            
	    }
	}
//    enable_irq(fts_wq_data->client->irq);
#if FTS_ESDCHECK_EN
    fts_esdcheck_set_intr(0);
#endif
	mutex_unlock(&fts_wq_data->report_mutex);
	//enable_irq(fts_wq_data->client->irq);

}
#endif
/*****************************************************************************
*  Name: fts_ts_interrupt
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static irqreturn_t fts_ts_interrupt(int irq, void *dev_id)
{
    struct fts_ts_data *fts_ts = dev_id;
    int ret = -1;
    if (!fts_ts)
    {
        FTS_ERROR("[INTR]: Invalid fts_ts");
        return IRQ_HANDLED;
    }
		//pr_err("[focal][touch]interrupt ! \n ");
//        disable_irq_nosync(fts_ts->client->irq);
/*#if FTS_INIT_WORK
    if (!fts_ts) {
        queue_work(fts_ts->ts_workqueue, &fts_ts->touch_event_work);        
    }

#else*/
	// disable_irq_nosync(fts_wq_data->client->irq);
    mutex_lock(&fts_wq_data->report_mutex);
    if(g_focal_touch_init_status == 1) {
        ret = fts_read_touchdata(fts_wq_data);
        if (((!cover_enable_touch_f) && (HALLsensor_gpio_value() == 0) && (!key_down_status)) || (ret != 0 || (disable_tp_flag)))
        {
			printk("[Focal][Touch] disable touch due to hall sensor CLOSE\n");
	    }
		else
		{
            fts_report_value(fts_wq_data);
            
	    }
    }
   
    mutex_unlock(&fts_wq_data->report_mutex);
    //enable_irq(fts_wq_data->client->irq);

    return IRQ_HANDLED;
}


/*****************************************************************************
*  Name: fts_creat_proc_interface
*  Brief: for check touch init status
*  Input:
*  Output:
*  Return:
*****************************************************************************/
    /* +++ ASUS proc touch Interface +++ */
    static int touch_proc_read(struct seq_file *buf, void *v)
    {
    
        if(g_focal_touch_init_status == 1) {
            seq_printf(buf, "%s\n", "OK");
            return 0;
        } else {
            seq_printf(buf, "%s\n", "FAIL");
            return 0;
        }
    
    }
    static int touch_proc_open(struct inode *inode, struct  file *file)
    {
        return single_open(file, touch_proc_read, NULL);
    }
    
    static void create_touch_proc_file(void)
    {
        static const struct file_operations proc_fops = {
            .owner = THIS_MODULE,
            .open =  touch_proc_open,
            .read = seq_read,
            .release = single_release,
        };
    
        struct proc_dir_entry *proc_file = proc_create("tpinit", 0444, NULL, &proc_fops);
    
        if (!proc_file) {
            printk("[PF]%s failed!\n", __FUNCTION__);
        }
        return;
    }
    
    /* --- ASUS proc touch Interface --- */



/*****************************************************************************
*  Name: fts_gpio_configure
*  Brief: Configure IRQ&RESET GPIO
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_gpio_configure(struct fts_ts_data *data)
{
    int err = 0;

    FTS_FUNC_ENTER();
    /* request irq gpio */
    if (gpio_is_valid(data->pdata->irq_gpio))
    {
        err = gpio_request(data->pdata->irq_gpio, "fts_irq_gpio");
        if (err)
        {
            FTS_ERROR("[GPIO]irq gpio request failed");
            goto err_irq_gpio_req;
        }

        err = gpio_direction_input(data->pdata->irq_gpio);
        if (err)
        {
            FTS_ERROR("[GPIO]set_direction for irq gpio failed");
            goto err_irq_gpio_dir;
        }
    }
    /* request reset gpio */
    if (gpio_is_valid(data->pdata->reset_gpio))
    {
        err = gpio_request(data->pdata->reset_gpio, "fts_reset_gpio");
        if (err)
        {
            FTS_ERROR("[GPIO]reset gpio request failed");
            goto err_irq_gpio_dir;
        }

        err = gpio_direction_output(data->pdata->reset_gpio, 1);
        if (err)
        {
            FTS_ERROR("[GPIO]set_direction for reset gpio failed");
            goto err_reset_gpio_dir;
        }
    }

    printk("[JK] irq %d to irq num is %d \n", data->pdata->irq_gpio, gpio_to_irq(data->pdata->irq_gpio));

    FTS_FUNC_EXIT();
    return 0;

err_reset_gpio_dir:
    if (gpio_is_valid(data->pdata->reset_gpio))
        gpio_free(data->pdata->reset_gpio);
err_irq_gpio_dir:
    if (gpio_is_valid(data->pdata->irq_gpio))
        gpio_free(data->pdata->irq_gpio);
err_irq_gpio_req:
    FTS_FUNC_EXIT();
    return err;
}


/*****************************************************************************
*  Name: fts_get_dt_coords
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_get_dt_coords(struct device *dev, char *name,
                             struct fts_ts_platform_data *pdata)
{
    u32 coords[FTS_COORDS_ARR_SIZE];
    struct property *prop;
    struct device_node *np = dev->of_node;
    int coords_size, rc;

    prop = of_find_property(np, name, NULL);
    if (!prop)
        return -EINVAL;
    if (!prop->value)
        return -ENODATA;


    coords_size = prop->length / sizeof(u32);
    if (coords_size != FTS_COORDS_ARR_SIZE)
    {
        FTS_ERROR("invalid %s", name);
        return -EINVAL;
    }

    rc = of_property_read_u32_array(np, name, coords, coords_size);
    if (rc && (rc != -EINVAL))
    {
        FTS_ERROR("Unable to read %s", name);
        return rc;
    }

    if (!strcmp(name, "focaltech,display-coords"))
    {
        pdata->x_min = coords[0];
        pdata->y_min = coords[1];
        pdata->x_max = coords[2];
        pdata->y_max = coords[3];
    }
    else
    {
        FTS_ERROR("unsupported property %s", name);
        return -EINVAL;
    }

    printk("[JK] print coords define X: from %d to %d , Y: from %d to %d \n", pdata->x_min, pdata->x_max, pdata->y_min, pdata->y_max);

    return 0;
}

/*****************************************************************************
*  Name: fts_parse_dt
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_parse_dt(struct device *dev, struct fts_ts_platform_data *pdata)
{
	//printk("[Focal][Touch]start parse DT! \n");
    int rc;
    struct device_node *np = dev->of_node;
    u32 temp_val;

    FTS_FUNC_ENTER();

    rc = fts_get_dt_coords(dev, "focaltech,display-coords", pdata);
    if (rc)
        FTS_ERROR("Unable to get display-coords");

    /* key */
    pdata->have_key = of_property_read_bool(np, "focaltech,have-key");
    if (pdata->have_key)
    {
        rc = of_property_read_u32(np, "focaltech,key-number", &pdata->key_number);
        if (rc)
        {
            FTS_ERROR("Key number undefined!");
        }
        rc = of_property_read_u32_array(np, "focaltech,keys",
                                        pdata->keys, pdata->key_number);
        if (rc)
        {
            FTS_ERROR("Keys undefined!");
        }
        rc = of_property_read_u32(np, "focaltech,key-y-coord", &pdata->key_y_coord);
        if (rc)
        {
            FTS_ERROR("Key Y Coord undefined!");
        }
        rc = of_property_read_u32_array(np, "focaltech,key-x-coord",
                                        pdata->key_x_coords, pdata->key_number);
        if (rc)
        {
            FTS_ERROR("Key X Coords undefined!");
        }
        FTS_DEBUG("%d: (%d, %d, %d), [%d, %d, %d][%d]",
                  pdata->key_number, pdata->keys[0], pdata->keys[1], pdata->keys[2],
                  pdata->key_x_coords[0], pdata->key_x_coords[1], pdata->key_x_coords[2],
                  pdata->key_y_coord);
    }

    /* reset, irq gpio info */
	pdata->reset_pin_status = 1;
    pdata->reset_gpio = of_get_named_gpio_flags(np, "focaltech,reset-gpio", 0, &pdata->reset_gpio_flags);
    if (pdata->reset_gpio < 0)
    {
        FTS_ERROR("Unable to get reset_gpio");
    }

    pdata->irq_gpio = of_get_named_gpio_flags(np, "focaltech,irq-gpio", 0, &pdata->irq_gpio_flags);
    if (pdata->irq_gpio < 0)
    {
        FTS_ERROR("Unable to get irq_gpio");
    }

    rc = of_property_read_u32(np, "focaltech,max-touch-number", &temp_val);
    if (!rc)
        pdata->max_touch_number = temp_val;
    else
        FTS_ERROR("Unable to get max-touch-number");
    printk("[JK] print gpio info reset pin: %d irq pin %d  \n", pdata->reset_gpio, pdata->irq_gpio);
    FTS_FUNC_EXIT();
    return 0;
}

/*#if defined(CONFIG_FB)
*****************************************************************************
*  Name: fb_notifier_callback
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************
static int focal_notifier_callback(struct notifier_block *self,
                                unsigned long event, void *data)
{
    struct fb_event *evdata = data;
    int *blank;

    struct fts_ts_data *fts_data =
        container_of(self, struct fts_ts_data, fb_notif);

        if (evdata && evdata->data && event == FB_EVENT_BLANK &&
            fts_data && fts_data->client)
        {
            blank = evdata->data;
            if (*blank == FB_BLANK_UNBLANK)
                //fts_ts_resume(&fts_data->client->dev);
                fts_ts_resume();
            else if (*blank == FB_BLANK_POWERDOWN)
                //fts_ts_suspend(&fts_data->client->dev);
                fts_ts_suspend();
        }
    return 0;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
*****************************************************************************
*  Name: fts_ts_early_suspend
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************
static void fts_ts_early_suspend(struct early_suspend *handler)
{
    struct fts_ts_data *data = container_of(handler,
                                            struct fts_ts_data,
                                            early_suspend);
	fts_ts_suspend();
  //  fts_ts_suspend(&data->client->dev);
}

*****************************************************************************
*  Name: fts_ts_late_resume
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************
static void fts_ts_late_resume(struct early_suspend *handler)
{
    struct fts_ts_data *data = container_of(handler,
                                            struct fts_ts_data,
                                            early_suspend);

   // fts_ts_resume(&data->client->dev);
   fts_ts_resume();
}
#endif


static struct notifier_block focal_noti_block = {
	.notifier_call = focal_notifier_callback,
};*/

static ssize_t fts_show_tpfwver(struct switch_dev *sdev, char *buf);

/*****************************************************************************
*  Name: fts_ts_probe
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct fts_ts_platform_data *pdata;
    struct fts_ts_data *data;
    struct input_dev *input_dev;
    int err = 0;
    int len;

  	if (g_asus_lcdID == 1){
		pr_err("[FTS][Touch] lcdid = %d is synaptic touch \n",g_asus_lcdID);
		return 0;
		}
	if (g_asus_lcdID == 0){
		pr_err("[FTS][Touch] lcdid = %d no input touch \n",g_asus_lcdID);
		return 0;
		}

	pr_err("[FTS][tocuh]fts_ts_probe! \n");

    /* 1. Get Platform data */


    if (client->dev.of_node)
    {


        pdata = kzalloc(sizeof(struct fts_ts_platform_data), GFP_KERNEL);
        if (!pdata)
        {
        
            pr_err("[FTS_err][tocuh][MEMORY]Failed to allocate memory \n");
            pr_err("[FTS][tocuh] fts_ts_prob_exit \n");
            return -ENOMEM;
        }
        err = fts_parse_dt(&client->dev, pdata);
        if (err)
        {
     		pr_err("[FTS][Touch] DT parsing failed! \n");
        }
    }
    else
    {
        pdata = client->dev.platform_data;
    }


    if (!pdata)
    {
        pr_err("[FTS_err][tocuh]Invalid pdata \n");
        pr_err("[FTS][tocuh] fts_ts_prob_exit \n");
        return -EINVAL;
    }

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        pr_err("[FTS_err][tocuh]I2C not supported\n");
         pr_err("[FTS][tocuh] fts_ts_prob_exit \n");
        return -ENODEV;
    }


    data = kzalloc(sizeof(struct fts_ts_data), GFP_KERNEL);
    if (!data)
    {
         pr_err("[FTS-err][tocuh][MEMORY]Failed to allocate memory\n");
        pr_err("[FTS][tocuh] fts_ts_prob_exit \n");
        return -ENOMEM;
    }

    input_dev = input_allocate_device();
    if (!input_dev)
    {
         pr_err("[FTS_err][tocuh][INPUT]Failed to allocate input device \n");
        pr_err("[FTS][tocuh] fts_ts_prob_exit \n");
        return -ENOMEM;
    }

    data->input_dev = input_dev;
    data->client = client;
    data->pdata = pdata;

    fts_wq_data = data;
    fts_i2c_client = client;
    fts_input_dev = input_dev;

    mutex_init(&fts_wq_data->report_mutex);
    
    /* Init and register Input device */
    input_dev->name = FTS_DRIVER_NAME;
    input_dev->id.bustype = BUS_I2C;
    input_dev->dev.parent = &client->dev;

    input_set_drvdata(input_dev, data);
    i2c_set_clientdata(client, data);

    __set_bit(EV_KEY, input_dev->evbit);
    if (data->pdata->have_key)
    {
         pr_err("[FTS][tocuh]set key capabilities \n");
        for (len = 0; len < data->pdata->key_number; len++)
        {
            input_set_capability(input_dev, EV_KEY, data->pdata->keys[len]);
        }
    }
    __set_bit(EV_ABS, input_dev->evbit);
    __set_bit(BTN_TOUCH, input_dev->keybit);
    __set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

#if FTS_MT_PROTOCOL_B_EN
    input_mt_init_slots(input_dev, pdata->max_touch_number, INPUT_MT_DIRECT);
#else
    input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 0x0f, 0, 0);
#endif
    input_set_abs_params(input_dev, ABS_MT_POSITION_X, pdata->x_min, pdata->x_max, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_POSITION_Y, pdata->y_min, pdata->y_max, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 0xFF, 0, 0);
#if FTS_REPORT_PRESSURE_EN
    input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 0xFF, 0, 0);
#endif
	input_set_capability(input_dev, EV_KEY, KEY_BACK);
	//input_set_capability(input_dev, EV_KEY, KEY_HOME);
	input_set_capability(input_dev, EV_KEY, KEY_MENU);

	__set_bit(KEY_BACK, input_dev->keybit);
	//__set_bit(KEY_HOME, input_dev->keybit);
	__set_bit(KEY_MENU, input_dev->keybit);
    err = input_register_device(input_dev);
    if (err)
    {
         pr_err("[FTS_err][tocuh]Input device registration failed \n");
        goto free_inputdev;
    }

#if FTS_POWER_SOURCE_CUST_EN
    fts_power_source_init(data);
    fts_power_source_ctrl(data, 1);
#endif

    err = fts_gpio_configure(data);
    if (err < 0)
    {
         pr_err("[FTS_err][tocuh][GPIO]Failed to configure the gpios \n");
        goto free_gpio;
    }
	fts_reset_proc(300);
	err = fts_wait_tp_to_valid(client);
    if (err < 0)
    {
        pr_err("[FTS_err][tocuh][GPIO]Failed to get TP Ready signal \n");
		goto free_gpio;
    }
	fts_wq_data->glove_mode_eable = 0;
	fts_wq_data->cover_mode_eable = 0;
	fts_wq_data->dclick_mode_eable = 0;
	fts_wq_data->swipeup_mode_eable = 0;
	fts_wq_data->gesture_mode_eable = 0;
	fts_wq_data->gesture_mode_type = 0;
#if 0
    INIT_WORK(&data->touch_event_work, fts_touch_irq_work);
    data->ts_workqueue = create_workqueue(FTS_WORKQUEUE_NAME);
    if (!data->ts_workqueue)
    {
        err = -ESRCH;
        pr_err("[FTS_err][tocuh] Create touch workqueue failed \n");
        goto exit_create_singlethread;
    }
#endif
  	data->suspend_resume_wq = create_singlethread_workqueue("focal_suspend_resume_wq");
	if (!data->suspend_resume_wq) {
		printk("[Focal][TOUCH_ERR] %s: create resume workqueue failed\n", __func__);
		goto err_create_wq_failed;
	}

	INIT_WORK(&data->resume_work, focal_resume_work);
//	INIT_WORK(&data->suspend_work, focal_suspend_work);
	
    fts_ctpm_get_upgrade_array();

    pr_err("[FTS][tocuh] peint irq num info %d \n", client->irq);

    err = request_threaded_irq(client->irq, NULL, fts_ts_interrupt,
                               pdata->irq_gpio_flags | IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
                               client->dev.driver->name, data);
    if (err)
    {
        pr_err("[FTS_err][tocuh] Request irq failed! \n");
        goto free_gpio;
    }

 //   disable_irq(client->irq);

#if FTS_PSENSOR_EN
    if ( fts_sensor_init(data) != 0)
    {
        pr_err("[FTS_err][tocuh] fts_sensor_init failed!");
        FTS_FUNC_EXIT();
        return 0;
    }
#endif

#if FTS_APK_NODE_EN
    fts_create_apk_debug_channel(client);
#endif

#if FTS_SYSFS_NODE_EN
    fts_create_sysfs(client);
#endif

    fts_ex_mode_init(client);

#if FTS_GESTURE_EN
    fts_gesture_init(input_dev, client);
#endif

#if FTS_ESDCHECK_EN
    fts_esdcheck_init();
#endif

#if FTS_AUTO_UPGRADE_EN
    fts_ctpm_upgrade_init();
#endif

    fts_update_fw_ver(data);
    fts_update_fw_vendor_id(data);
	data->touch_sdev.name = "touch";
	data->touch_sdev.print_name = fts_show_tpfwver;

	if (switch_dev_register(&data->touch_sdev) < 0)
	{
		printk("[Focal][TOUCH_ERR] %s: failed to register switch_dev \n", __func__);
		goto exit_err_sdev_register_fail;
	} 
    pr_err("[FTS][tocuh]Firmware version = 0x%02x.%d.%d, fw_vendor_id=0x%02x \n",
             data->fw_ver[0], data->fw_ver[1], data->fw_ver[2], data->fw_vendor_id);


#if FTS_TEST_EN
    fts_test_init(client);
#endif

create_touch_proc_file();


/*#if defined(CONFIG_FB)

    data->fb_notif = focal_noti_block;


    //data->fb_notif.notifier_call = fb_notifier_callback;

    err = fb_register_client(&data->fb_notif);
    if (err)
        pr_err("[FTS_err][tocuh] Unable to register fb_notifier: %d \n", err);

#elif defined(CONFIG_HAS_EARLYSUSPEND)
    data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + FTS_SUSPEND_LEVEL;
    data->early_suspend.suspend = fts_ts_early_suspend;
    data->early_suspend.resume = fts_ts_late_resume;
    register_early_suspend(&data->early_suspend);
#endif*/

    g_focal_touch_init_status = 1;

    enable_irq(client->irq);

    FTS_FUNC_EXIT();
    return 0;
exit_err_sdev_register_fail:
#if 0
exit_create_singlethread:
    FTS_ERROR("==singlethread error =");
    i2c_set_clientdata(client, NULL);
#endif
err_create_wq_failed:
	if (data->suspend_resume_wq) {
		destroy_workqueue(data->suspend_resume_wq);
	}
free_gpio:
    if (gpio_is_valid(data->pdata->reset_gpio))
        gpio_free(data->pdata->reset_gpio);
    if (gpio_is_valid(data->pdata->irq_gpio))
        gpio_free(data->pdata->irq_gpio);
free_inputdev:
    input_free_device(input_dev);
    pr_err("[FTS][tocuh] fts_ts_prob_exit \n");
    return err;
}
static ssize_t fts_show_tpfwver(struct switch_dev *sdev, char *buf)
{
	int num_read_chars = 0;
	IC_FW = get_focal_tp_fw();
	g_vendor_id=fts_wq_data->fw_vendor_id;
	if (IC_FW == 255) {
		printk("[Focal][Touch] %s :  read FW fail \n ", __func__);
		num_read_chars = snprintf(buf, PAGE_SIZE, "get tp fw version fail!\n");
	} else {
		printk("[Focal][Touch] %s :  TP_ID = 0x%x touch FW = 0x%x\n ", __func__, g_vendor_id, IC_FW);
		num_read_chars = snprintf(buf, PAGE_SIZE, "0x%x-0x%x\n", g_vendor_id, IC_FW);
	}
	return num_read_chars;
}
u8 get_focal_tp_fw(void)
{
	u8 fwver = 0;
	if (fts_i2c_read_reg(fts_i2c_client, FTS_REG_FW_VER, &fwver) < 0)
		return -1;
	else
		return fwver;
}
/*int ftxxxx_read_tp_id(void)
{

	int err = 0;
	wake_lock(&ps_lock1);

	mutex_lock(&fts_input_dev->mutex);

	disable_irq(fts_wq_data->client->irq);
		err = fts_ctpm_fw_upgrade_ReadVendorID(fts_wq_data->client, &B_VenderID);
		printk("[touch]bootloader vender id= %x \n",B_VenderID);
		if (err < 0)
			B_VenderID = 0xFF;
	printk("[Focal][Touch] %s : TP Bootloadr info : vendor ID = %x !\n", __func__, B_VenderID);

//	asus_check_touch_mode();

	mutex_unlock(&fts_input_dev->mutex);

    wake_unlock(&ps_lock1);
    enable_irq(fts_wq_data->client->irq);

	return B_VenderID;
}*/




/*****************************************************************************
*  Name: fts_ts_remove
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int fts_ts_remove(struct i2c_client *client)
{
    struct fts_ts_data *data = i2c_get_clientdata(client);

    FTS_FUNC_ENTER();
    cancel_work_sync(&data->touch_event_work);
    destroy_workqueue(data->ts_workqueue);
	if (data->suspend_resume_wq) {
		destroy_workqueue(data->suspend_resume_wq);
	}
#if FTS_PSENSOR_EN
    fts_sensor_remove(data);
#endif

#if FTS_APK_NODE_EN
    fts_release_apk_debug_channel();
#endif

#if FTS_SYSFS_NODE_EN
    fts_remove_sysfs(fts_i2c_client);
#endif

    fts_ex_mode_exit(client);

#if FTS_AUTO_UPGRADE_EN
    cancel_work_sync(&fw_update_work);
#endif

#if defined(CONFIG_FB)
    if (fb_unregister_client(&data->fb_notif))
        FTS_ERROR("Error occurred while unregistering fb_notifier.");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    unregister_early_suspend(&data->early_suspend);
#endif
    free_irq(client->irq, data);

    if (gpio_is_valid(data->pdata->reset_gpio))
        gpio_free(data->pdata->reset_gpio);

    if (gpio_is_valid(data->pdata->irq_gpio))
        gpio_free(data->pdata->irq_gpio);

    input_unregister_device(data->input_dev);

#if FTS_TEST_EN
    fts_test_exit(client);
#endif

#if FTS_ESDCHECK_EN
    fts_esdcheck_exit();
#endif

    FTS_FUNC_EXIT();
    return 0;
}
void fts_ts_shutdown(struct i2c_client *client)
{	
	if (g_focal_touch_init_status==1){
		struct fts_ts_data *ftxxxx_ts;
	ftxxxx_ts = i2c_get_clientdata(client);
    if (gpio_is_valid(ftxxxx_ts->pdata->irq_gpio)) {
        free_irq(client->irq, ftxxxx_ts);
        gpio_free(ftxxxx_ts->pdata->irq_gpio);
    }
	gpio_direction_output(fts_wq_data->pdata->reset_gpio, 0);
	gpio_set_value(ftxxxx_ts->pdata->reset_gpio, 0);
	printk("[touch] reset_gpio =0\n");
		}
	else
		printk("[FTS][touch]%s: focal touch not work skip showtown \n", __func__);
	
	
}


/*****************************************************************************
*  Name: fts_ts_suspend
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
int fts_ts_suspend(void)
{	
	if (g_focal_touch_init_status==0){
		printk("[FTS][touch]%s: focal touch not work skip suspend \n", __func__);
			}
	if (g_focal_touch_init_status==1) {
		focal_suspend_work();
		//queue_work(fts_wq_data->suspend_resume_wq, &fts_wq_data->suspend_work);
	}
	return 0;
		
}
EXPORT_SYMBOL(fts_ts_suspend);
	/*****************************************************************************
	*  Name: fts_ts_resume
	*  Brief:
	*  Input:
	*  Output:
	*  Return:
	*****************************************************************************/

int fts_ts_resume(void)
{
	if (g_focal_touch_init_status==0){
		printk("[FTS][touch]%s: focal touch not work skip resume \n", __func__);
	}
	if (g_focal_touch_init_status==1) {
	queue_work(fts_wq_data->suspend_resume_wq, &fts_wq_data->resume_work);
	}
	return 0;

}
EXPORT_SYMBOL(fts_ts_resume);

bool fts_gesture_check(void)
{
	return ((fts_wq_data->dclick_mode_eable == 1) | (fts_wq_data->swipeup_mode_eable == 1) | (fts_wq_data->gesture_mode_eable == 1));
}
EXPORT_SYMBOL(fts_gesture_check);

/*static int fts_ts_suspend(struct device *dev)
{
    struct fts_ts_data *data = dev_get_drvdata(dev);
    int retval = 0;

    FTS_FUNC_ENTER();
    if (data->suspended)
    {
        FTS_INFO("Already in suspend state");
        FTS_FUNC_EXIT();
        return -1;
    }

    fts_release_all_finger();

#if FTS_GESTURE_EN
    retval = fts_gesture_suspend(data->client);
    if (retval == 0)
    {
        //Enter into gesture mode(suspend) 
        retval = enable_irq_wake(fts_wq_data->client->irq);
        if (retval)
            FTS_ERROR("%s: set_irq_wake failed", __func__);
        data->suspended = true;
        FTS_FUNC_EXIT();
        return 0;
    }
#endif

#if FTS_PSENSOR_EN
    if ( fts_sensor_suspend(data) != 0 )
    {
        enable_irq_wake(data->client->irq);
        data->suspended = true;
        return 0;
    }
#endif

#if FTS_ESDCHECK_EN
    fts_esdcheck_suspend();
#endif

    disable_irq_nosync(data->client->irq);
    // TP enter sleep mode 
    retval = fts_i2c_write_reg(data->client, FTS_REG_POWER_MODE, FTS_REG_POWER_MODE_SLEEP_VALUE);
    if (retval < 0)
    {
        FTS_ERROR("Set TP to sleep mode fail, ret=%d!", retval);
    }
    data->suspended = true;

    FTS_FUNC_EXIT();

    return 0;
}*/

/*****************************************************************************
*  Name: fts_ts_resume
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
/*static int fts_ts_resume(struct device *dev)
{
    struct fts_ts_data *data = dev_get_drvdata(dev);
    int err;
    char val;

    FTS_FUNC_ENTER();
    if (!data->suspended)
    {
        FTS_DEBUG("Already in awake state");
        FTS_FUNC_EXIT();
        return -1;
    }

#if FTS_GESTURE_EN
    if (fts_gesture_resume(data->client) == 0)
    {
        err = disable_irq_wake(data->client->irq);
        if (err)
            FTS_ERROR("%s: disable_irq_wake failed",__func__);
        data->suspended = false;
        FTS_FUNC_EXIT();
        return 0;
    }
#endif

#if (!FTS_CHIP_IDC)
    fts_reset_proc(200);
#endif

	fts_tp_state_recovery(data->client);

#if FTS_PSENSOR_EN
    if ( fts_sensor_resume(data) != 0 )
    {
        disable_irq_wake(data->client->irq);
        data->suspended = false;
        FTS_FUNC_EXIT();
        return 0;
    }
#endif

    err = fts_i2c_read_reg(fts_i2c_client, 0xA6, &val);
    FTS_DEBUG("read 0xA6: %02X, ret: %d", val, err);

    data->suspended = false;

    enable_irq(data->client->irq);

#if FTS_ESDCHECK_EN
    fts_esdcheck_resume();
#endif
    FTS_FUNC_EXIT();
    return 0;
}*/

/*****************************************************************************
* I2C Driver
*****************************************************************************/
static const struct i2c_device_id fts_ts_id[] =
{
    {FTS_DRIVER_NAME, 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, fts_ts_id);

static struct of_device_id fts_match_table[] =
{
    { .compatible = "focaltech,fts", },
    { },
};

static struct i2c_driver fts_ts_driver =
{
    .probe = fts_ts_probe,
    .remove = fts_ts_remove,
    .shutdown =fts_ts_shutdown,
    .driver = {
        .name = FTS_DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = fts_match_table,
    },
    .id_table = fts_ts_id,
};

/*****************************************************************************
*  Name: fts_ts_init
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int __init fts_ts_init(void)
{
    int ret = 0;

    printk(KERN_ERR"[JK] Focal init start  \n");

//    FTS_FUNC_ENTER();
    ret = i2c_add_driver(&fts_ts_driver);
    if ( ret != 0 )
    {
        printk(KERN_ERR"[JK] check 1 \n");
    }
        printk(KERN_ERR"[JK] Focal init end  \n");
//    FTS_FUNC_EXIT();
    return ret;
}

/*****************************************************************************
*  Name: fts_ts_exit
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void __exit fts_ts_exit(void)
{
    i2c_del_driver(&fts_ts_driver);
}

module_init(fts_ts_init);
module_exit(fts_ts_exit);

MODULE_AUTHOR("FocalTech Driver Team");
MODULE_DESCRIPTION("FocalTech Touchscreen Driver");
MODULE_LICENSE("GPL v2");
