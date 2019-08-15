#ifndef  FAC_CAMERA_H_INCLUDED
#define FAC_CAMERA_H_INCLUDED

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time.h>

#include "msm_sensor.h"

#define  sheldon_debug      printk
#define  HADES_OTP_MODE   //define for hades rear otp read

#define SENSOR_ID_IMX298  0x0298
#define SENSOR_ID_T4K37   0x1c21
#define SENSOR_ID_T4K35   0x1481
#define SENSOR_ID_OV5670  0x5670
#define SENSOR_ID_OV8856  0x885a
#define SENSOR_ID_IMX362  0x0362
#define SENSOR_ID_S5K3M3  0x30D3
#define SENSOR_ID_IMX214  0x0214
#define SENSOR_ID_IMX351  0x0351
#define SENSOR_ID_IMX319  0x0319

#define OTP_DATA_LEN_WORD (32)
#define OTP_DATA_LEN_BYTE (64)

#define	PROC_FILE_STATUS_REAR_1  "driver/rear_otp"
#define	PROC_FILE_STATUS_REAR_2  "driver/rear2_otp"
#define	PROC_FILE_STATUS_FRONT	"driver/front_otp"

#define	PROC_FILE_STATUS_BANK1_REAR_1	"driver/rear_otp_bank1"
#define	PROC_FILE_STATUS_BANK1_REAR_2	"driver/rear2_otp_bank1"
#define	PROC_FILE_STATUS_BANK1_FRONT	"driver/front_otp_bank1"

#define	PROC_FILE_STATUS_BANK2_REAR_1	"driver/rear_otp_bank2"
#define	PROC_FILE_STATUS_BANK2_REAR_2	"driver/rear2_otp_bank2"
#define	PROC_FILE_STATUS_BANK2_FRONT	"driver/front_otp_bank2"

#define	PROC_FILE_STATUS_BANK3_REAR_1	"driver/rear_otp_bank3"
#define	PROC_FILE_STATUS_BANK3_REAR_2	"driver/rear2_otp_bank3"
#define	PROC_FILE_STATUS_BANK3_FRONT	"driver/front_otp_bank3"

#define	PROC_FILE_REARMODULE_1	"driver/RearModule"
#define	PROC_FILE_REARMODULE_2	"driver/RearModule2"
#define	PROC_FILE_FRONTMODULE	"driver/FrontModule"

#define REAR_PROC_FILE_PDAF_INFO	"driver/PDAF_value_more_info"
#define FRONT_PROC_FILE_PDAF_INFO	"driver/PDAF_value_more_info_1"

//for thermal team
#define   PROC_FILE_THERMAL_REAR  "driver/camera_temp"
#define   PROC_FILE_THERMAL_REAR2  "driver/camera2_temp"
#define   PROC_FILE_THERMAL_FRONT "driver/camera_temp_front"

//for DIT
#define   PROC_FILE_THERMAL_WIDE  "driver/rear_temp"
#define   PROC_FILE_THERMAL_TELE  "driver/rear2_temp"
#define   PROC_FILE_EEPEOM_THERMAL_WIDE  "driver/rear_eeprom_temp"
#define   PROC_FILE_EEPEOM_THERMAL_TELE  "driver/rear2_eeprom_temp"

#define   PROC_FILE_EEPEOM_ARCSOFT  "driver/dualcam_cali"

#define	PROC_FILE_VCM_VERSION	"driver/rear_otp_vcm" //ASUS_BSP Lucien +++: Using service to auto update ois fw

void set_sensor_info(int camera_id,struct msm_sensor_ctrl_t* ctrl,uint16_t sensor_id);
void create_proc_otp_thermal_file(struct msm_camera_sensor_slave_info *slave_info); 
void create_otp_bank_1_proc_file(int cameraID);
void create_otp_bank_2_proc_file(int cameraID);
void create_otp_bank_3_proc_file(int cameraID);
void create_otp_vcm_proc_file(int cameraID);//ASUS_BSP Lucien +++: Using service to auto update ois fw

 void remove_bank1_file(void);
 void remove_bank2_file(void);
 void remove_bank3_file(void);
 void remove_vcm_file(void); //ASUS_BSP Lucien +++: Using service to auto update ois fw

 void create_proc_pdaf_info(void);
 void clear_proc_pdaf_info(void);
 void remove_file(void);//otp & pdaf

//sheldon++
 int fcamera_power_up( struct msm_sensor_ctrl_t *s_ctrl);
 int fcamera_power_down( struct msm_sensor_ctrl_t *s_ctrl);

 uint16_t sensor_read_temperature(uint16_t sensor_id, int cameraID);
 void create_thermal_file(int camID, int sensorID);
 void remove_thermal_file(void);

 void create_dump_eeprom_file(int camID, int sensorID);

int ov8856_otp_read(int camNum);
int t4k37_35_otp_read(void);
int s5k3m3_otp_read(int camNum);
int imx362_otp_read(uint16_t camNum);
int imx214_otp_read(int camNum);
int read_otp_asus(int sensor_id, int cameraID);
//sheldon--

  int rear_1_module_proc_read(struct seq_file *buf, void *v);
 int rear_2_module_proc_read(struct seq_file *buf, void *v);
  int front_module_proc_read(struct seq_file *buf, void *v);
 void create_rear_module_proc_file(int cameraID);
 void create_front_module_proc_file(void);
 void remove_module_file(void);

  ssize_t rear_1_status_proc_read(struct device *dev, struct device_attribute *attr, char *buf);
  ssize_t rear_2_status_proc_read(struct device *dev, struct device_attribute *attr, char *buf);
  ssize_t front_status_proc_read(struct device *dev, struct device_attribute *attr, char *buf);
 int create_rear_status_proc_file(int cameraID);
 int create_front_status_proc_file(void);
 void remove_proc_file(void); //status

  ssize_t rear_1_resolution_read(struct device *dev, struct device_attribute *attr, char *buf);
  ssize_t rear_2_resolution_read(struct device *dev, struct device_attribute *attr, char *buf);
  ssize_t front_resolution_read(struct device *dev, struct device_attribute *attr, char *buf);
 int create_rear_resolution_proc_file(int cameraID);
 int create_front_resolution_proc_file(void);
 void remove_resolution_file(void); //resolution
 extern int32_t ois_read_rear_vcm_version(uint32_t mode, uint8_t vcm); //ASUS_BSP Lucien +++: Replace update ois fw after reading vcm data

#endif
