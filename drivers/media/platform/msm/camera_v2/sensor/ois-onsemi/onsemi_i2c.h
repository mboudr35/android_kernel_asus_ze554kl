#ifndef ASUS_OIS_ONSEMI_I2C_H
#define ASUS_OIS_ONSEMI_I2C_H
#include <linux/types.h>
#include "msm_ois.h"
#include "onsemi_ois_api.h"

int onsemi_read_byte(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t* reg_data);
int onsemi_read_word(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t* reg_data);
int onsemi_read_dword(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint32_t* reg_data);
int onsemi_read_seq_bytes(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint8_t* reg_data, uint32_t size);

int onsemi_poll_byte(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t reg_data, uint32_t delay_ms);
int onsemi_poll_word(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t reg_data, uint32_t delay_ms);

int onsemi_write_byte(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t reg_data);
int onsemi_write_word(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t reg_data);
int onsemi_write_dword(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint32_t reg_data);
int onsemi_write_seq_bytes(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint8_t* reg_data,uint32_t size);
int onsemi_write_table_cmd(struct msm_ois_ctrl_t * ctrl,struct msm_camera_i2c_reg_array *reg_data, uint8_t size);

void RamWrite32A( struct msm_ois_ctrl_t * ctrl, uint16_t reg_addr, uint32_t reg_data );
void RamRead32A( struct msm_ois_ctrl_t * ctrl, uint16_t reg_addr, uint32_t* reg_data );
int CntWrt( struct msm_ois_ctrl_t * ctrl, uint8_t* data, uint32_t len);
#endif
