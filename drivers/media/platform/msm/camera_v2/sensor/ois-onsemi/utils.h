#ifndef ASUS_OIS_UTILS_H
#define ASUS_OIS_UTILS_H
#include <linux/types.h> //type def uintxx in msm_ois.h
#include <linux/delay.h>
#include <soc/qcom/camera2.h>

void swap_word_data(uint16_t* register_data);
int format_hex_string(char *output_buf,int len, uint8_t* data,int count);

void delay_ms(uint32_t time);
void delay_us(uint32_t time);
void	WitTim( uint16_t time);

int64_t diff_time_us(struct timeval *t1, struct timeval *t2 );

uint64_t get_file_size(const char *filename);
int read_file_into_buffer(const char *filename, uint8_t* data, uint32_t size);

int sysfs_read_byte_seq(char *filename, uint8_t *value, uint32_t size);
int sysfs_read_char_seq(char *filename, int *value, uint32_t size);
int sysfs_read_word_seq(char *filename, int *value, uint32_t size);
bool sysfs_write_byte_seq(char *filename, uint8_t *value, uint32_t size);
bool sysfs_write_word_seq(char *filename, uint16_t *value, uint32_t size);
bool sysfs_write_word_seq_change_line(char *filename, uint16_t *value, uint32_t size,uint32_t number);
bool sysfs_write_dword_seq_change_line(char *filename, uint32_t *value, uint32_t size,uint32_t number);

void fix_gpio_value_of_power_down_setting(struct msm_camera_power_ctrl_t *power_info);
bool is_i2c_seq_setting_address_valid(struct msm_camera_i2c_seq_reg_setting* i2c_seq_setting, uint16_t max_reg_addr);
bool i2c_seq_setting_contain_address(struct msm_camera_i2c_seq_reg_setting* i2c_seq_setting, uint16_t reg_addr, uint8_t *reg_val);

#endif
