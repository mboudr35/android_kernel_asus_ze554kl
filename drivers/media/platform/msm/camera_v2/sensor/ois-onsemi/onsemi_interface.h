#ifndef ASUS_OIS_ONSEMI_INTERFACE_H
#define ASUS_OIS_ONSEMI_INTERFACE_H

#include "msm_ois.h"
#include "onsemi_ois_api.h"

#define SECTOR_SIZE				320
#define BLOCK_UNIT				0x200
#define BLOCK_BYTE				2560
#define BURST_LENGTH  ( 8*5 )

#define	USER_RESERVE			3		// Reserved for customer data blocks
#define	ERASE_BLOCKS			(16 - USER_RESERVE)
#define   FLASH_ACCESS_SIZE		32

typedef enum
{
  OIS_DATA,
  PID_PARAM,
  USER_DATA
}flash_write_data_type;

typedef enum
{
	INIT_STATE=0,
	IDLE_STATE,
	RUN_STATE,
	HALL_CALI_STATE,
	HALL_POLA_STATE,
	GYRO_CALI_STATE,
	RESERVED1_STATE,
	RESERVED2_STATE,
	PWM_DUTY_FIX_STATE,
	DFLS_UPDATE_STATE,//user data area update state
	STANDBY_STATE,//low power consumption state
	GYRO_COMM_CHECK_STATE,
	RESERVED3_STATE,
	RESERVED4_STATE,
	LOOP_GAIN_CTRL_STATE,
	RESERVED5_STATE,
	GYRO_SELF_TEST_STATE,
	OIS_DATA_WRITE_STATE,
	PID_PARAM_INIT_STATE,
	GYRO_WAKEUP_WAIT_STATE
}onsemi_ois_state;

onsemi_ois_state onsemi_get_state(struct msm_ois_ctrl_t * ctrl);
const char * onsemi_get_state_str(onsemi_ois_state state);

int onsemi_idle2standy_state(struct msm_ois_ctrl_t * ctrl);
int onsemi_standy2idle_state(struct msm_ois_ctrl_t * ctrl);

int onsemi_servo_go_on(struct msm_ois_ctrl_t * ctrl);
int onsemi_servo_go_off(struct msm_ois_ctrl_t * ctrl);

int onsemi_get_servo_state(struct msm_ois_ctrl_t * ctrl, int *state);
int onsemi_restore_servo_state(struct msm_ois_ctrl_t * ctrl, int state);

int onsemi_fw_info_enable(struct msm_ois_ctrl_t * ctrl);
int onsemi_fw_info_disable(struct msm_ois_ctrl_t * ctrl);

int onsemi_switch_mode(struct msm_ois_ctrl_t *ctrl, uint8_t mode);

int onsemi_set_i2c_mode(struct msm_ois_ctrl_t *ctrl,enum i2c_freq_mode_t mode);

int onsemi_gyro_read_data(struct msm_ois_ctrl_t * ctrl,uint16_t* gyro_data, uint32_t size);
int onsemi_gyro_read_K_related_data(struct msm_ois_ctrl_t * ctrl,uint16_t* gyro_data, uint32_t size);
//ASUS_BSP Lucien +++: Save one Gyro data after doing OIS calibration
int onsemi_gyro_read_xy(struct msm_ois_ctrl_t * ctrl,uint32_t *x_value, uint32_t *y_value);
//ASUS_BSP Lucien ---: Save one Gyro data after doing OIS calibration

int onsemi_read_pair_sensor_data(struct msm_ois_ctrl_t * ctrl,
								 uint32_t reg_addr_x,uint32_t reg_addr_y,
								 uint32_t *value_x,uint32_t *value_y);

int onsemi_read_acc_position(struct msm_ois_ctrl_t * ctrl,uint16_t *x_value, uint16_t *y_value);


uint8_t onsemi_gyro_calibration(struct msm_ois_ctrl_t * ctrl, stReCalib *pReCalib);

int onsemi_gyro_communication_check(struct msm_ois_ctrl_t * ctrl);

int onsemi_gyro_adjust_gain(struct msm_ois_ctrl_t * ctrl);
int onsemi_hall_calibration(struct msm_ois_ctrl_t * ctrl);
int onsemi_hall_check_polarity(struct msm_ois_ctrl_t * ctrl);
int onsemi_measure_loop_gain(struct msm_ois_ctrl_t * ctrl);
int onsemi_adjust_loop_gain(struct msm_ois_ctrl_t * ctrl);

int onsemi_check_flash_write_result(struct msm_ois_ctrl_t * ctrl,flash_write_data_type type);


int onsemi_init_PID_params(struct msm_ois_ctrl_t * ctrl);
int onsemi_init_all_params(struct msm_ois_ctrl_t * ctrl);

int onsemi_backup_ois_data_to_file(struct msm_ois_ctrl_t * ctrl, char * file);
int onsemi_restore_ois_data_from_file(struct msm_ois_ctrl_t * ctrl, char * file);

int onsemi_backup_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_addr, uint32_t size, uint8_t *buffer);
int onsemi_compare_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_addr, uint32_t size, uint8_t *buffer);

int onsemi_write_user_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_offset,uint8_t* data, uint32_t size);
int onsemi_read_user_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_offset,uint8_t* data, uint32_t size);

int onsemi_update_fw(struct msm_ois_ctrl_t *ctrl, const char *file_path);
int onsemi_update_parameter_for_updating_fw(struct msm_ois_ctrl_t *ctrl);
//ASUS_BSP Lucien +++: Replace update ois fw after reading vcm data
int onsemi_update_fw_from_eeprom(struct msm_ois_ctrl_t *ctrl, uint32_t mode, uint8_t vcm);
//ASUS_BSP Lucien ---: Replace update ois fw after reading vcm data

int onsemi_ssc_go_on(struct msm_ois_ctrl_t *ctrl);
int onsemi_ssc_go_off(struct msm_ois_ctrl_t *ctrl);
int onsemi_af_dac_setting(struct msm_ois_ctrl_t *ctrl, uint32_t val);

void IORead32A( struct msm_ois_ctrl_t *ctrl, uint32_t IOadrs, uint32_t*IOdata );
void IOWrite32A(struct msm_ois_ctrl_t *ctrl, uint32_t IOadrs, uint32_t IOdata );

#endif
