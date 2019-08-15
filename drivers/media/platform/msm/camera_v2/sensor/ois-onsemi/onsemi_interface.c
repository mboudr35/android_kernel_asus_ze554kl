#include "onsemi_interface.h"
#include "onsemi_i2c.h"
#include "utils.h"

#include	"FromCode_01_01.h"    //ASUS_BSP Lucien +++: SEMCO module FW header
#include	"FromCode_05_00.h"    //ASUS_BSP Lucien +++: Primax module FW header
#include	"FromCode_05_01.h"    //ASUS_BSP Lucien +++: Primax new module FW header
#include	"onsemi_ois_api.h"
#include	"onsemi_ois_pmemcode.h"

#undef  pr_fmt
#define pr_fmt(fmt) " OIS-onsemi %s() " fmt, __func__

uint32_t	UlBufDat[ 64 ] ;

//**************************
//	Table of download file
//**************************

const DOWNLOAD_TBL DTbl[] = {
	{0x0500, CcMagicCodeF40_05_00, sizeof(CcMagicCodeF40_05_00), CcFromCodeF40_05_00, sizeof(CcFromCodeF40_05_00) },
	{0x0501, CcMagicCodeF40_05_01, sizeof(CcMagicCodeF40_05_01), CcFromCodeF40_05_01, sizeof(CcFromCodeF40_05_01) },
	{0x0101, CcMagicCodeF40_01_01, sizeof(CcMagicCodeF40_01_01), CcFromCodeF40_01_01, sizeof(CcFromCodeF40_01_01) },
	{0xFFFF, (void*)0,                0,                               (void*)0,               0                  }
};

static bool onsemi_is_idle_state(struct msm_ois_ctrl_t * ctrl);


static int onsemi_is_servo_on(struct msm_ois_ctrl_t * ctrl);
static int onsemi_servo_on(struct msm_ois_ctrl_t * ctrl);
static int onsemi_servo_off(struct msm_ois_ctrl_t * ctrl);

static int onsemi_wait_value_with_timeout(struct msm_ois_ctrl_t * ctrl, uint16_t reg_addr, uint16_t mask, uint16_t expected_value, uint32_t timeout_value, const char* func_name);

//static int onsemi_sw_reset(struct msm_ois_ctrl_t * ctrl);

static int onsemi_switch_mode_internal(struct msm_ois_ctrl_t *ctrl, uint8_t mode, uint32_t *reg_addr, uint32_t *reg_data);

int onsemi_is_servo_on(struct msm_ois_ctrl_t * ctrl)
{
       int rc;
	uint32_t reg_data = 0;
	uint32_t check_id = 0x5130101;

	rc = onsemi_read_dword(ctrl,0x8000,&reg_data);
	pr_info(" reg_data = %04x\n", reg_data ) ;

	if( rc < 0 || reg_data != check_id)
	{
		pr_info("Ois is power off now.\n");
		return 0;
	}
	else if(reg_data == check_id)
	{
		pr_info("Ois is power on now.\n");
		return 1;
	}
	else
		pr_info("Unknow failed!\n");

	return rc;
}

int onsemi_servo_on(struct msm_ois_ctrl_t * ctrl)
{
	uint8_t UcStRd = 1;

	RamWrite32A( ctrl, 0xF012 , 0x00000001 ) ;
	while( UcStRd ) {
		UcStRd = F40_RdStatus(ctrl, 1);
	}
       pr_info(" onsemi_servo_on( Status) = %02x\n", UcStRd ) ;
	//int rc = onsemi_write_dword(ctrl,0xF012,1);
	return UcStRd;
}
int onsemi_servo_off(struct msm_ois_ctrl_t * ctrl)
{
	uint8_t UcStRd = 1;

	RamWrite32A( ctrl, 0xF012 , 0x00000000 ) ;
	while( UcStRd ) {
		UcStRd = F40_RdStatus(ctrl, 1);
	}
       pr_info(" onsemi_servo_off( Status) = %02x\n", UcStRd ) ;
	//int rc = onsemi_write_dword(ctrl,0xF012,0);
	return UcStRd;
}

int onsemi_servo_go_on(struct msm_ois_ctrl_t * ctrl)
{
	if(onsemi_is_servo_on(ctrl))
	{
		return onsemi_servo_on(ctrl);
	}
	return 0;
}


int onsemi_servo_go_off(struct msm_ois_ctrl_t * ctrl)
{
	if(onsemi_is_servo_on(ctrl))
	{
		return onsemi_servo_off(ctrl);
	}
	return 0;
}


int onsemi_get_servo_state(struct msm_ois_ctrl_t * ctrl, int *state)
{
	if(onsemi_is_servo_on(ctrl))
	{
		*state = 1;
	}
	else
	{
		*state = 0;
	}

	return 0;
}

int onsemi_restore_servo_state(struct msm_ois_ctrl_t * ctrl, int state)
{
	int rc;
	int current_state = -1;
	uint32_t val;

	rc = onsemi_get_servo_state(ctrl,&current_state);
	if(current_state != state)
	{
		val = (state?0x1:0x0);
		rc = onsemi_write_dword(ctrl,0xF012,val);
		delay_ms(10);
	}
	else
	{
		//pr_info("current state %d equal to new state, not set command\n",current_state);
		rc = 0;
	}
	return rc;
}

int onsemi_ssc_go_on(struct msm_ois_ctrl_t *ctrl)
{
       uint8_t UcStRd = 1;

	RamWrite32A( ctrl, 0xF01C , 0x00000001 ) ;
	while( UcStRd ) {
		UcStRd = F40_RdStatus(ctrl,1);
	}
       pr_info(" SscEna( Status) = %02x\n", UcStRd ) ;
	return 0;
}

int onsemi_ssc_go_off(struct msm_ois_ctrl_t *ctrl)
{
	uint8_t UcStRd = 1;

	RamWrite32A( ctrl, 0xF01C , 0x00000000 ) ;
	while( UcStRd ) {
		UcStRd = F40_RdStatus(ctrl,1);
	}
       pr_info(" SscDis( Status) = %02x\n", UcStRd ) ;
	return 0;
}

int onsemi_af_dac_setting(struct msm_ois_ctrl_t *ctrl, uint32_t val)
{
	uint8_t UcStRd = 1;
	pr_info(" val = %08x\n", val ) ;
	RamWrite32A( ctrl, 0xF01A , val ) ;
	while( UcStRd ) {
		UcStRd = F40_RdStatus(ctrl,1);
	}
       pr_info(" onsemi_af_dac_setting( Status) = %02x\n", UcStRd ) ;
	return 0;
}
int onsemi_switch_mode(struct msm_ois_ctrl_t *ctrl, uint8_t mode)
{
	int rc = 0;
	//uint32_t reg_addr = 0;
	//uint32_t reg_data = 0;
	uint32_t *reg_addr;
	uint32_t *reg_data;

	reg_addr = kzalloc(sizeof(uint32_t)*2,GFP_KERNEL);
	reg_data = kzalloc(sizeof(uint32_t)*2,GFP_KERNEL);
	switch(mode)
	{
		case 0:
			reg_addr[0] = 0xF012;
			reg_data[0] = 0x00000000; //onsemi centering mode
			break;
		case 1:
			reg_addr[0] = 0xF013;
			reg_data[0] = 0x00000000; //onsemi move mode
			break;
		case 2:
			reg_addr[0] = 0xF013;
			reg_data[0] = 0x00000001; //onsemi still mode
			break;
		default:
			pr_err("Not supported ois mode %d\n",mode);
			rc = -1;
	}
	if(rc == 0)
	{
               pr_info("Make sure servo and ssc on before change mode");
               rc = onsemi_servo_on(ctrl);//OIS on
               rc = onsemi_ssc_go_on(ctrl);//ssc on
		if(mode == 0 ) pr_info("Mode changed to center\n");
		if(mode == 1 ) pr_info("Mode changed to movie\n");
		if(mode == 2 ) pr_info("Mode changed to still\n");

		rc = onsemi_switch_mode_internal(ctrl,mode, reg_addr, reg_data);
	}
	kfree(reg_addr);
	kfree(reg_data);
	reg_addr = NULL;
	reg_data = NULL;
	return rc;
}

static int onsemi_switch_mode_internal(struct msm_ois_ctrl_t *ctrl, uint8_t mode, uint32_t *reg_addr, uint32_t *reg_data)
{
       int rc;
	uint8_t UcStRd = 1;

	if(mode < 3)
	{
		rc = onsemi_write_dword(ctrl, reg_addr[0], reg_data[0]);
		while( UcStRd ) {
			UcStRd = F40_RdStatus(ctrl, 1);
		}

		if( rc < 0) pr_err("setting mode failed\n");
	}
	else
	{
		pr_err("invalid mode %d for onsemi-lc898123f40, valid value is [0,2]\n",mode);
		rc = -1;
	}

	return rc;
}

int onsemi_set_i2c_mode(struct msm_ois_ctrl_t *ctrl,enum i2c_freq_mode_t mode)
{
	int rc;
	uint16_t reg_data = 0xeeee;
	uint16_t verify_data = 0xeeee;
	switch(mode)
	{
		case I2C_STANDARD_MODE:
			 reg_data = 0x2;
			 break;
		case I2C_FAST_MODE:
			 reg_data = 0x0;
			 break;
		case I2C_FAST_PLUS_MODE:
			 reg_data = 0x1;
			 break;
		default:
			 pr_err("not supported i2c mode %d, set to FAST MODE",mode);
			 reg_data = 0x0;//default set to 0x0
			 break;
	}
	rc = onsemi_write_byte(ctrl,0x03FD,reg_data);
	if(rc == 0)
	{
		rc = onsemi_read_byte(ctrl,0x03FD,&verify_data);
		if(verify_data == reg_data)
		{
			pr_info("Set I2C Mode Succeeded!\n");
			rc = 0;
		}
		else
		{
			pr_err("read back i2c mode 0x%x not equal with set i2c mode 0x%x\n",
					verify_data,reg_data
			);
			rc = -1;
		}
	}
	return rc;
}

int onsemi_backup_ois_data_to_file(struct msm_ois_ctrl_t * ctrl, char * file)
{
	bool rc = false;
	uint8_t * pOisData;
	pOisData = kzalloc(sizeof(uint8_t)*1024,GFP_KERNEL);
	if (!pOisData)
	{
		pr_err("no memory!\n");
		return -1;
	}
	rc =onsemi_read_seq_bytes(ctrl,0x0200,pOisData,1024);
	if(rc == 0)
		rc = sysfs_write_byte_seq(file,pOisData,1024);
	else
		pr_err("read 0x0200 ois data failed!\n");

	kfree(pOisData);
	if(rc == true)
	{
		rc = 0;
	}
	else
	{
		rc = -2;
	}
	return rc;
}

int onsemi_restore_ois_data_from_file(struct msm_ois_ctrl_t * ctrl, char * file)
{
	int rc;
	uint8_t * pOisData;
	int old_state, i;
	pOisData = kzalloc(sizeof(uint8_t)*1024,GFP_KERNEL);//scanf(%x) require to be uint32
	if (!pOisData)
	{
		pr_err("no memory!\n");
		return -1;
	}

	rc = sysfs_read_byte_seq(file,pOisData,1024);

	if(rc > 0)
	{
	/*
	index[124], reg 0x27c changed, 0x0 -> 0x94
	index[125], reg 0x27d changed, 0x0 -> 0xbf
	index[126], reg 0x27e changed, 0x2 -> 0x3
	index[148], reg 0x294 changed, 0x0 -> 0x8e
	index[149], reg 0x295 changed, 0x50 -> 0x34
	index[150], reg 0x296 changed, 0x1 -> 0x3
	*/

	/*
	 index[14], reg 0x20e changed, 0x3d -> 0x3a
	 index[15], reg 0x20f changed, 0x40 -> 0x41
	index[16], reg 0x210 changed, 0x6f -> 0x70
	 index[17], reg 0x211 changed, 0x6a -> 0x63
	index[18], reg 0x212 changed, 0x3e -> 0x45
	 index[20], reg 0x214 changed, 0xa2 -> 0xb4
	index[22], reg 0x216 changed, 0x20 -> 0x10
	index[24], reg 0x218 changed, 0x85 -> 0x83
	 index[26], reg 0x21a changed, 0x0 -> 0xfc
	 index[27], reg 0x21b changed, 0x8 -> 0x7
	 index[28], reg 0x21c changed, 0x0 -> 0xc9
	  index[29], reg 0x21d changed, 0x8 -> 0x7
	*/
#if 0
		//X9B
		rc = onsemi_write_seq_bytes(ctrl,0x027C,pOisData+124,4);
		//Y9B
		if(rc == 0)
			rc = onsemi_write_seq_bytes(ctrl,0x0294,pOisData+148,4);

		//14~29
		if(rc == 0)
			rc = onsemi_write_seq_bytes(ctrl,0x020E,pOisData+14,16);
#else
               for(i=0;i<8;i++) { //every time write 128 bytes, total write 1024 bytes
                       //rc = onsemi_write_byte(ctrl,0x0200+i,pOisData[i]);//some registers can not be written...changed after power u
                       rc = onsemi_write_seq_bytes(ctrl,0x0200+i*128,pOisData+i*128,128);//some registers can not be written...changed
               }
#endif
		if(rc == 0)
		{
			onsemi_get_servo_state(ctrl,&old_state);
			onsemi_servo_go_off(ctrl);
			rc = onsemi_check_flash_write_result(ctrl,OIS_DATA);
			onsemi_restore_servo_state(ctrl,old_state);
		}
	}
	else
	{
		pr_err("Read ois data from %s failed, rc = %d\n",file,rc);
	}

	kfree(pOisData);

	return rc;
}

int onsemi_backup_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_addr, uint32_t size, uint8_t *buffer)
{
	return onsemi_read_seq_bytes(ctrl,reg_addr,buffer,size);
}

int onsemi_compare_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_addr, uint32_t size, uint8_t *buffer)
{
	uint8_t * pNewData = NULL;
	uint8_t * pOldData = buffer;
	int i;
	int rc = 1;
	pNewData = kzalloc(sizeof(uint8_t)*size,GFP_KERNEL);
	if (!pNewData)
	{
		pr_err("no memory!\n");
		return -ENOMEM;
	}

	onsemi_read_seq_bytes(ctrl,reg_addr,pNewData,size);

	for(i=0;i<size;i++)
	{
		if(pNewData[i] != pOldData[i])
		{
			pr_info("index[%d], reg 0x%04x changed, 0x%02x -> 0x%02x\n",
			i, reg_addr+i,
			pOldData[i],pNewData[i]
			);
			rc = 0;
		}
	}

	kfree(pNewData);

	return rc;
}
int onsemi_gyro_adjust_gain(struct msm_ois_ctrl_t * ctrl)
{
	int rc;
	int index = 0;
	static uint32_t float_value_integer_map[] =
	{
		0x3f800000,//1.0f
		0x3fc00000,//1.5f
		0x40000000,//2.0f
		0x40200000,//2.5f
		0x40400000,//3.0f
		0x40600000,//3.5f
		0x40800000,//4.0f
		0x40900000,//4.5f
		0x40a00000,//5.0f
		0x40b00000,//5.5f
		0x40c00000,//6.0f
		0x40d00000,//6.5f
		0x40e00000,//7.0f
		0x40f00000//7.5f
	};

	static uint32_t integer_values[14];
	/*OIS On*/
	rc = onsemi_write_byte(ctrl,0x0002,0x10);//Mode 0

	rc = onsemi_write_byte(ctrl,0x0000,0x01);//OIS EN set

	//Gyro Gain Tunning X
	for(index=0;index<sizeof(float_value_integer_map)/sizeof(uint32_t);index++)
	{
		rc = onsemi_write_dword(ctrl,0x0254,float_value_integer_map[index]);//XGG register, uint32 as float
		rc = onsemi_write_byte(ctrl,0x0015,0x10);//GGA CTRL XGGEN=1
		delay_ms(500);

		onsemi_read_dword(ctrl,0x0254,&integer_values[index]);
		/*
		if()//WHAT EXPRESSION ?
		{
			break;
		}
		*/
	}
#if 0
	for(index = 0;index < 14;index++)
	{
		pr_info("read dword value is 0x%x\n",integer_values[index]);
	}
#endif
	//Gyro Gain Tunning Y
	for(index=0;index<sizeof(float_value_integer_map)/sizeof(uint32_t);index++)
	{
		rc = onsemi_write_dword(ctrl,0x0258,float_value_integer_map[index]);//XGG register, uint32 as float
		rc = onsemi_write_byte(ctrl,0x0015,0x20);//GGA CTRL YGGEN=1
		delay_ms(500);
		/*
		if()//WHAT EXPRESSION ?
		{
			break;
		}
		*/
	}
	/*Lens Cotrol End*/

	rc = onsemi_write_byte(ctrl,0x0000,0x00);//OIS CTRL OIS EN clear

	return rc;

}
int onsemi_gyro_read_data(struct msm_ois_ctrl_t * ctrl,uint16_t* gyro_data, uint32_t size)
{
	int rc;
	int old_servo_state;
	uint16_t gyro_x_config,gyro_y_config;
	uint16_t gyro_x_value,gyro_y_value;
	uint16_t gyro_min,gyro_max,gyro_x_K,gyro_y_K;

	uint16_t acc_x,acc_y;
	if(size<10)
	{
		pr_err("size %d not enough to hold gyro data\n",size);
		return -1;
	}

	onsemi_get_servo_state(ctrl,&old_servo_state);

	onsemi_servo_go_on(ctrl);

	onsemi_fw_info_enable(ctrl);

	onsemi_read_word(ctrl,0x0082,&gyro_x_value);
	onsemi_read_word(ctrl,0x0084,&gyro_y_value);

	onsemi_read_acc_position(ctrl,&acc_x,&acc_y);

	onsemi_fw_info_disable(ctrl);

	onsemi_servo_off(ctrl);

	if(!onsemi_is_idle_state(ctrl))
		return -2;

	rc = onsemi_write_byte(ctrl,0x00F5,0x00);//gyro sensor register address set
	rc = onsemi_write_byte(ctrl,0x00F4,0x01);//gyro sensor register read start
	rc = onsemi_wait_value_with_timeout(ctrl,0x00F4,0x01,0,10000,__func__);
	if(rc < 0)
	{
		pr_err("Read Gyro Sensor Register X Config Data Fail!\n");
		return -3;
	}

	if(rc >= 0)
	{
		rc = onsemi_read_byte(ctrl,0x00F6,&gyro_x_config);
		if(rc >=0)
		{
			pr_info("Got gyro x config data 0x%x\n",gyro_x_config);
		}
	}

	rc = onsemi_write_byte(ctrl,0x00F5,0x01);//gyro sensor register address set
	rc = onsemi_write_byte(ctrl,0x00F4,0x01);//gyro sensor register read start
	rc = onsemi_wait_value_with_timeout(ctrl,0x00F4,0x01,0,10000,__func__);
	if(rc < 0)
	{
		pr_err("Read Gyro Sensor Register Y Config Data Fail!\n");
		return -4;
	}

	if(rc >= 0)
	{
		rc = onsemi_read_byte(ctrl,0x00F6,&gyro_y_config);
		if(rc >=0)
		{
			pr_info("Got gyro y config data 0x%x\n",gyro_y_config);
		}
	}

	onsemi_read_word(ctrl,0x0244,&gyro_max);
	onsemi_read_word(ctrl,0x0246,&gyro_min);
	onsemi_read_word(ctrl,0x0248,&gyro_x_K);
	onsemi_read_word(ctrl,0x024A,&gyro_y_K);

	pr_info("Got gyro value(%d, %d), min %d, max %d, x_K %d, y_K %d, config(0x%x, 0x%x)\n",
			(int16_t)gyro_x_value,(int16_t)gyro_y_value,
			(int16_t)gyro_min,(int16_t)gyro_max,
			(int16_t)gyro_x_K,(int16_t)gyro_y_K,
			gyro_x_config,gyro_x_config
			);
	gyro_data[0] = gyro_x_value;
	gyro_data[1] = gyro_y_value;
	gyro_data[2] = gyro_min;
	gyro_data[3] = gyro_max;
	gyro_data[4] = gyro_x_K;
	gyro_data[5] = gyro_y_K;
	gyro_data[6] = gyro_x_config;
	gyro_data[7] = gyro_y_config;

	gyro_data[8] = acc_x;
	gyro_data[9] = acc_y;

	onsemi_restore_servo_state(ctrl,old_servo_state);

	return rc;
}

int onsemi_gyro_read_K_related_data(struct msm_ois_ctrl_t * ctrl,uint16_t* gyro_data, uint32_t size)
{
	int rc;
	int old_servo_state;

	uint16_t gyro_x_K,gyro_y_K;

	if(size<2)
	{
		pr_err("size %d not enough to carry gyro data, should be 2\n",size);
		return -1;
	}

	onsemi_get_servo_state(ctrl,&old_servo_state);

	rc = onsemi_read_word(ctrl,0x0248,&gyro_x_K);
	if(rc == 0)
		rc = onsemi_read_word(ctrl,0x024A,&gyro_y_K);

	onsemi_restore_servo_state(ctrl,old_servo_state);

	gyro_data[0] = gyro_x_K;
	gyro_data[1] = gyro_y_K;

	return rc;
}

int onsemi_fw_info_enable(struct msm_ois_ctrl_t * ctrl)
{
	int rc = onsemi_write_byte(ctrl,0x0080,0x01);//FWINFO_CTRL Enable
	delay_ms(2);
	return rc;
}
int onsemi_fw_info_disable(struct msm_ois_ctrl_t * ctrl)
{
	int rc = onsemi_write_byte(ctrl,0x0080,0x00);//FWINFO_CTRL Disable
	delay_ms(2);
	return rc;
}
//ASUS_BSP Lucien +++: Save one Gyro data after doing OIS calibration
int onsemi_gyro_read_xy(struct msm_ois_ctrl_t * ctrl,uint32_t *x_value, uint32_t *y_value)
{
	int rc;

	rc = onsemi_read_dword(ctrl,0x0278,x_value);
	if(rc == 0)
		rc = onsemi_read_dword(ctrl,0x027C,y_value);
	return rc;
}
//ASUS_BSP Lucien ---: Save one Gyro data after doing OIS calibration
int onsemi_read_acc_position(struct msm_ois_ctrl_t * ctrl,uint16_t *x_value, uint16_t *y_value)
{
	int rc;

	rc = onsemi_read_word(ctrl,0x044C,x_value);
	if(rc == 0)
		rc = onsemi_read_word(ctrl,0x044E,y_value);

	return rc;
}
int onsemi_read_pair_sensor_data(struct msm_ois_ctrl_t * ctrl,
								 uint32_t reg_addr_x,uint32_t reg_addr_y,
								 uint32_t *value_x,uint32_t *value_y)
{

	int rc;
	rc = onsemi_read_dword(ctrl,reg_addr_x,value_x);
	if(rc == 0)
		rc = onsemi_read_dword(ctrl,reg_addr_y,value_y);
	return rc;
}

uint8_t onsemi_gyro_calibration(struct msm_ois_ctrl_t * ctrl, stReCalib *pReCalib)
{
	uint8_t result;

	result = F40_GyroReCalib(ctrl, pReCalib);
	pr_info("F40_GyroReCalib result = 0x%2x", result);

	if(!result)
	{
	     pr_err("F40_GyroReCalib success\n");
	     if(pReCalib->SsDiffX > 0x1000 || pReCalib->SsDiffY > 0x1000) return FAILURE;

	     //Go to write gyro data
            result = F40_WrGyroOffsetData(ctrl);
	     if(!result)
	     {
	        pr_err("F40_WrGyroOffsetData success\n");
		 return SUCCESS;
	     }
	     else
	     {
	        pr_err("F40_WrGyroOffsetData result = 0x%02x\n", result);
		 return FAILURE;
	     }
	}
	else
	{
	      if(result == 0x02) pr_err("Axis X error\n");
	      if(result == 0x04) pr_err("Axis Y error\n");
	      if(result == 0x06) pr_err("Axis X, Y error\n");

	      return result;
	}

}
int onsemi_hall_check_polarity(struct msm_ois_ctrl_t * ctrl)
{
	int rc;
	uint16_t reg_data = 0xeeee;

	if(!onsemi_is_idle_state(ctrl))
		return -1;

	rc = onsemi_write_byte(ctrl,0x0012,0x01);/* HPCTRL HPEN set */

	rc = onsemi_wait_value_with_timeout(ctrl,0x0012,0xff,0,10000,__func__);
	if(rc < 0)
	{
		pr_err("Hall Check Polarity Not End ?!\n");
		return rc;
	}

	rc = onsemi_read_byte(ctrl,0x0200,&reg_data);

	if( (reg_data & 0x0C ) == 0x0 )
	{
		pr_err("Hall check Polarity Complete!\n");
		rc = onsemi_check_flash_write_result(ctrl,OIS_DATA);
	}
	else
	{
		pr_err("Hall check Polarity Failed!\n");
		rc = -1;
	}
	return rc;

}

int onsemi_hall_calibration(struct msm_ois_ctrl_t * ctrl)
{
	int rc;
	uint16_t reg_data = 0xeeee;

	if(!onsemi_is_idle_state(ctrl))
		return -1;

	rc = onsemi_write_byte(ctrl,0x0013,0x01);/* HCCTRL HCEN set */

	rc = onsemi_wait_value_with_timeout(ctrl,0x0013,0xff,0,10000,__func__);
	if(rc < 0)
	{
		pr_err("Hall Calibration Not End ?!\n");
		return rc;
	}

	rc = onsemi_read_byte(ctrl,0x04,&reg_data);/* OISERR Read */

	if( (reg_data & 0x0c) == 0x0 )
	{
		pr_info("Hall Calibration Complete!\n");
		rc = onsemi_check_flash_write_result(ctrl,OIS_DATA);
	}
	else
	{
		pr_info("Hall Calibration Failed! reg_data 0x%x\n",reg_data);
		rc = -1;
		//Todo Change Hall Parameter
	}

	return rc;
}

int onsemi_check_flash_write_result(struct msm_ois_ctrl_t * ctrl,flash_write_data_type type)
{
	uint16_t reg_data = 0xeeee;
	int rc;

	if(type != OIS_DATA && type != PID_PARAM && type != USER_DATA)
	{
		pr_err("Not supported check type %d!\n",type);
		return -1;
	}

	/* Set Result Check Data */
	rc = onsemi_write_byte(ctrl,0x0027,0xAA);


	switch(type)
	{
		case OIS_DATA:
			/* Exec OIS DATA AREA Write */
			rc = onsemi_write_byte(ctrl,0x0003,0x01);/* set OIS_W = 1 */
			delay_ms(170);/* Wait for Flash ROM Write */
			rc = onsemi_wait_value_with_timeout(ctrl,0x0003,0xff,0,10000,__func__);
			if(rc < 0)
			{
				pr_err("Flash ROM Write Not End?!\n");
				return -1;
			}
			break;

		case PID_PARAM:
			/* Exec PID parameter Init */
			rc = onsemi_write_byte(ctrl,0x0036,0x01);/* PIDPARAMINIT INIT_EN set */
			delay_ms(170); /* Wait for Flash ROM Write */
			rc = onsemi_wait_value_with_timeout(ctrl,0x0036,0xff,0,10000,__func__);
			if(rc < 0)
			{
				pr_err("Flash ROM Write Not End?!\n");
				return -1;
			}
			break;

		case USER_DATA:
			/* Exec User Data Area Write */
			rc = onsemi_write_byte(ctrl,0x000E,0x07);
			delay_ms(170); /* Wait for Flash ROM Write */
			rc = onsemi_wait_value_with_timeout(ctrl,0x000E,0xff,0x17,10000,__func__);
			if(rc < 0)
			{
				pr_err("Flash ROM Write Not End?!\n");
				return -1;
			}
			break;

		default:
			pr_err("Not supported check type %d!\n",type);
	}

	rc = onsemi_read_byte(ctrl,0x0027,&reg_data);

	if(reg_data != 0x00AA )
	{
		/* Flash Write Error */
		pr_err("Flash Write FAIL!\n");
		rc = -1;
	}
	else
	{
		/* Flash Write Success */
		pr_info("Flash Write OK!\n");
		rc = 0;
	}
	return rc;
}

int onsemi_gyro_communication_check(struct msm_ois_ctrl_t * ctrl)
{
	uint16_t reg_data = 0xeeee;
	int rc;

	if(!onsemi_is_idle_state(ctrl))
		return -1;

	/* Gyro Calibration Start */
	onsemi_write_byte(ctrl,0x0014,0x02);//GCCTRL GSCEN set, calibration start

	rc = onsemi_wait_value_with_timeout(ctrl,0x0014,0xff,0,10000,__func__);
	if(rc < 0)
	{
		pr_err("Calibration Sequence Not End ?\n");
		return rc;
	}

	rc = onsemi_read_byte(ctrl,0x0004,&reg_data);
	if( (reg_data & 0x20) != 0x0 )
	{
		pr_err("Communication error!\n");
		rc = -1;
	}
	else
	{
		pr_info("Communication OK!\n");
		rc = 0;
	}
	return rc;
	/* OISERR register GCOMERR Bit != 0(Communication Error Found!!) */
}
#if 0
static int onsemi_sw_reset(struct msm_ois_ctrl_t * ctrl)
{
	int rc = 0;

	/* onsemi SW Reset Sequence */
	onsemi_write_byte(ctrl,0x000D,0x01);

	rc = onsemi_wait_value_with_timeout(ctrl,0x0001,0xff,0x9,10000,__func__);
	if(rc < 0)
	{
		pr_err("OISSTS State not 0x9 ?\n");
		return rc;
	}

	/* 0x9 == DFLS_UPDATE STATE */

	/* DFLSCMD=6 (Reset) */
	onsemi_write_byte(ctrl,0x000E,0x06);

	delay_ms(20);

	return rc;
}
#endif
int onsemi_measure_loop_gain(struct msm_ois_ctrl_t * ctrl)
{
	int rc;

	uint16_t read_data_short[4];
	uint8_t  read_data_char[8];
	if(!onsemi_is_idle_state(ctrl))
	{
		return -1;
	}
	/*set setting parameter(example of configuration)*/
	rc = onsemi_write_byte(ctrl,0x00D1,0x64);//LGFREQ set
	rc = onsemi_write_byte(ctrl,0x00D2,0x03);//LGNUM set
	rc = onsemi_write_byte(ctrl,0x00D3,0x04);//LGSKIPNUM set
	rc = onsemi_write_word(ctrl,0x00D4,0x0049);//LGAMP set

	/*X-axis loop gain measurement start*/
	rc = onsemi_write_byte(ctrl,0x00D0,0x01);//LGCTRL LGSEL(0:X-axis) and LGEN set

	/*check end*/
	rc = onsemi_wait_value_with_timeout(ctrl,0x00D0,0xff,0x0,10000,__func__);
	if(rc == 0)
	{
		/*check result*/
		rc = onsemi_read_seq_bytes(ctrl,0x00C0,read_data_char,8);//LGMON1AMP,LGMON2AMP,LGMON1PHASE,LGMON2PHASE

		read_data_short[0] = (uint16_t)(read_data_char[1] << 8) | read_data_char[0];
		read_data_short[1] = (uint16_t)(read_data_char[3] << 8) | read_data_char[2];
		read_data_short[2] = (uint16_t)(read_data_char[5] << 8) | read_data_char[4];
		read_data_short[3] = (uint16_t)(read_data_char[7] << 8) | read_data_char[6];

		pr_info("X loop gain measure, got 0x%x 0x%x 0x%x 0x%x\n",
			read_data_short[0],
			read_data_short[1],
			read_data_short[2],
			read_data_short[3]
		);
	}
	else
	{
		pr_err("X-axis loop gain measurement NOT END!\n");
		return -2;
	}

	/*Y-axis loop gain measurement start*/
	rc = onsemi_write_byte(ctrl,0x00D0,0x11);//LGCTRL LGSEL(1:Y-axis) and LGEN set

	/*check end*/
	rc = onsemi_wait_value_with_timeout(ctrl,0x00D0,0xff,0x10,10000,__func__);
	if(rc == 0)
	{
		/*check result*/
		rc = onsemi_read_seq_bytes(ctrl,0x00C0,read_data_char,8);

		read_data_short[0] = (uint16_t)(read_data_char[1] << 8) | read_data_char[0];
		read_data_short[1] = (uint16_t)(read_data_char[3] << 8) | read_data_char[2];
		read_data_short[2] = (uint16_t)(read_data_char[5] << 8) | read_data_char[4];
		read_data_short[3] = (uint16_t)(read_data_char[7] << 8) | read_data_char[6];

		pr_info("Y loop gain measure, got 0x%x 0x%x 0x%x 0x%x\n",
			read_data_short[0],
			read_data_short[1],
			read_data_short[2],
			read_data_short[3]
		);
	}
	else
	{
		pr_err("Y-axis loop gain measurement NOT END!\n");
		return -3;
	}
	return rc;
}

int onsemi_init_all_params(struct msm_ois_ctrl_t * ctrl)
{
	int rc;
	int old_servo_state;

	onsemi_get_servo_state(ctrl,&old_servo_state);

	onsemi_servo_go_off(ctrl);

	if(!onsemi_is_idle_state(ctrl))
		return -2;

	rc = onsemi_write_byte(ctrl,0x0036,0x81);//all params initialization enabled

	delay_ms(170);

	rc = onsemi_wait_value_with_timeout(ctrl,0x0036,0xff,0x00,10000,__func__);
	if(rc < 0)
	{
		pr_err("all params initialization not end?!\n");
	}
	else
	{
		pr_info("all params initialization done!\n");
	}

	onsemi_restore_servo_state(ctrl,old_servo_state);

	return rc;
}
int onsemi_init_PID_params(struct msm_ois_ctrl_t * ctrl)
{
	int rc;
	int old_servo_state;
	uint32_t X9B,Y9B;

	rc = onsemi_read_dword(ctrl,0x027C,&X9B);
	rc = onsemi_read_dword(ctrl,0x0294,&Y9B);

	if( X9B == 0x20000 && Y9B == 0x15000 )
	{
		pr_info("X9B and Y9B are init values, no need to INIT PID Params\n");
		return 0;
	}
	else
	{
		pr_info("X9B 0x%08x, Y9B 0x%08x, pair are not init values, need to INIT PID Params\n",X9B,Y9B);
	}

	onsemi_get_servo_state(ctrl,&old_servo_state);

	onsemi_servo_go_off(ctrl);

	if(!onsemi_is_idle_state(ctrl))
		return -2;

	rc = onsemi_write_byte(ctrl,0x0036,0x01);//PID initialization enabled

	delay_ms(170);

	rc = onsemi_wait_value_with_timeout(ctrl,0x0036,0xff,0x00,10000,__func__);
	if(rc < 0)
	{
		pr_err("PID initialization not end?!\n");
	}
	else
	{
		pr_info("PID initialization done!\n");
	}

	onsemi_restore_servo_state(ctrl,old_servo_state);

	return rc;
}

int onsemi_adjust_loop_gain(struct msm_ois_ctrl_t * ctrl)
{
	int rc;

	if(!onsemi_is_idle_state(ctrl))
		return -1;

	/*set setting parameter(example of configuration)*/
	rc = onsemi_write_byte(ctrl,0x00D1,0x64);//LGFREQ set
	rc = onsemi_write_byte(ctrl,0x00D2,0x03);//LGNUM set
	rc = onsemi_write_byte(ctrl,0x00D3,0x04);//LGSKIPNUM set
	rc = onsemi_write_word(ctrl,0x00D4,0x0049);//LGAMP set
	rc = onsemi_write_dword(ctrl,0x00C8,0x418E432A);//LGTGX set
	rc = onsemi_write_dword(ctrl,0x00CC,0x41B33333);//LGTGY set

	//X-axis loop gain adjustment start
	rc = onsemi_write_byte(ctrl,0x00D0,0x03);//LGCTRL LGSEL(0:X-axis), LGAEN and LGEN set

	//check end
	rc = onsemi_wait_value_with_timeout(ctrl,0x00D0,0xff,0x02,10000,__func__);
	if(rc < 0)
	{
		pr_err("X-axis loop gain adjustment failed!\n");
		rc = -2;
	}
	rc = onsemi_write_byte(ctrl,0x00D0,0x13);//LGCTRL LGSEL(1:Y-axis),LGAEN and LGEN set

	rc = onsemi_wait_value_with_timeout(ctrl,0x00D0,0xff,0x12,10000,__func__);
	if(rc < 0)
	{
		pr_err("Y-axis loop gain adjustment failed!\n");
	}
	if(rc == 0)
		rc = onsemi_check_flash_write_result(ctrl,OIS_DATA);

	return rc;

}



#define STRFORMAT(enum_name) case enum_name: return ""#enum_name"";

const char * onsemi_get_state_str(onsemi_ois_state state)
{
	switch(state)
	{
		STRFORMAT(INIT_STATE)
		STRFORMAT(IDLE_STATE)
		STRFORMAT(RUN_STATE)
		STRFORMAT(HALL_CALI_STATE)
		STRFORMAT(HALL_POLA_STATE)
		STRFORMAT(GYRO_CALI_STATE)
		STRFORMAT(RESERVED1_STATE)
		STRFORMAT(RESERVED2_STATE)
		STRFORMAT(PWM_DUTY_FIX_STATE)
		STRFORMAT(DFLS_UPDATE_STATE)//user data area update state
		STRFORMAT(STANDBY_STATE)//low power consumption state
		STRFORMAT(GYRO_COMM_CHECK_STATE)
		STRFORMAT(RESERVED3_STATE)
		STRFORMAT(RESERVED4_STATE)
		STRFORMAT(LOOP_GAIN_CTRL_STATE)
		STRFORMAT(RESERVED5_STATE)
		STRFORMAT(GYRO_SELF_TEST_STATE)
		STRFORMAT(OIS_DATA_WRITE_STATE)
		STRFORMAT(PID_PARAM_INIT_STATE)
		STRFORMAT(GYRO_WAKEUP_WAIT_STATE)
		default:
		return "Unknown State";
	}
}

onsemi_ois_state onsemi_get_state(struct msm_ois_ctrl_t * ctrl)
{
	uint16_t reg_data = 0xeeee;
	int rc;

	/* OIS Status Check */
	rc = onsemi_read_byte(ctrl,0x0001,&reg_data);
	if(rc == 0)
		return (onsemi_ois_state)reg_data;
	else
		return 0xffff;
}

bool onsemi_is_idle_state(struct msm_ois_ctrl_t * ctrl)
{
	uint16_t reg_data = 0xeeee;
	int rc;

	/* OIS Status Check */
	rc = onsemi_read_byte(ctrl,0x0001,&reg_data);
	pr_info("onsemi state now is %s\n",onsemi_get_state_str((onsemi_ois_state)reg_data));

	if(rc == 0)
		return reg_data == 0x01;
	else
		return false;
}

int onsemi_idle2standy_state(struct msm_ois_ctrl_t * ctrl)
{
	if(!onsemi_is_idle_state(ctrl))
		return -1;

	return onsemi_write_byte(ctrl,0x0030,0x01);//POWER CTRL STANDBY EN set
}
int onsemi_standy2idle_state(struct msm_ois_ctrl_t * ctrl)
{
	return onsemi_wait_value_with_timeout(ctrl,0x0001,0xff,1,10000,__func__);
}

static int onsemi_wait_value_with_timeout(struct msm_ois_ctrl_t * ctrl, uint16_t reg_addr, uint16_t mask, uint16_t expected_value, uint32_t timeout_value,const char * func_name)
{
	int rc = 0;
	int count = 0;
	uint16_t reg_data;

	do
	{
		count ++;
		if(count > 1)
		{
			delay_ms(1);
		}
		rc = onsemi_read_byte(ctrl,reg_addr,&reg_data);
		if(count % 300 == 0)
			pr_info("%s() read reg 0x%04x %d times, reg_data 0x%02x\n",func_name,reg_addr,count,reg_data);

	}while( (reg_data & mask) != expected_value && count < timeout_value && rc >= 0 );

	if((reg_data & mask) == expected_value)
	{
		pr_info("%s() Got expected value 0x%x (mask 0x%x, key_value 0x%x) from reg 0x%04x, times %d\n",
			func_name,
			reg_data,
			mask,
			expected_value,
			reg_addr,
			count
		);
		rc = 0;
	}
	else if(count >= timeout_value)
	{
		pr_err("%s() Time out! Excceed %d ms\n",func_name,timeout_value);
		rc = -1;
	}
	else
	{
		pr_err("%s() I2C is Bad?!\n",func_name);
		rc = -2;
	}
	return rc;
}


int onsemi_write_user_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_offset,uint8_t* data, uint32_t size)
{
	uint16_t reg_data = 0xeeee;
	uint8_t * log_buffer = NULL;
	int rc;

	if(reg_offset + size/4 -1 > 0xFF)
	{
		pr_err("offset 0x%x + size %d, target offset reg address 0x%x too large, max offset value is 0xFF!\n",
		reg_offset,
		size,
		reg_offset + size/4);
		return -1;
	}

	if(!onsemi_is_idle_state(ctrl))
	{
		return -2;
	}

	onsemi_write_byte(ctrl,0x000D,0x01);//Set DFLSEN=1

	if(onsemi_get_state(ctrl) != DFLS_UPDATE_STATE)
	{
		pr_err("Enter User Data Area Write Mode Failed! State now is %s\n",
		onsemi_get_state_str(onsemi_get_state(ctrl))
		);
		onsemi_write_byte(ctrl,0x000E,0x05);//END Command Set
		return -3;
	}

	/*User Data Write*/

	onsemi_write_byte(ctrl,0x000F,size/4);
	onsemi_write_word(ctrl,0x0010,reg_offset);//0x7400~0x74FF
	onsemi_write_seq_bytes(ctrl,0x0100,data,size);

	onsemi_write_byte(ctrl,0x000E,0x01);//DFLSCMD WRITE

	onsemi_read_byte(ctrl,0x000E,&reg_data);

	if(reg_data == 0x11)
	{
		pr_info("Write User Data Complete! offset 0x%x, size %d\n",
			reg_offset,
			size
		);
		rc = onsemi_check_flash_write_result(ctrl,USER_DATA);
		if(rc == 0)
		{
			log_buffer = kzalloc(sizeof(uint8_t)*size*8,GFP_KERNEL);
			if (!log_buffer)
			{
				pr_err("no memory!\n");
				onsemi_write_byte(ctrl,0x000E,0x05);//END Command Set
				return -ENOMEM;
			}
			if(format_hex_string(log_buffer,sizeof(uint8_t)*size*8,data,size)!= -1)
			{
				pr_info("Write User Data as follows, size %d\n",size);
				pr_info("%s",log_buffer);
			}

			kfree(log_buffer);
		}
	}
	else
	{
		pr_err("Write user data command failed!\n");
		rc = -4;
	}
	onsemi_write_byte(ctrl,0x000E,0x05);//END Command Set
	return rc;

}
int onsemi_read_user_data(struct msm_ois_ctrl_t *ctrl,uint16_t reg_offset,uint8_t* data, uint32_t size)
{
	int rc;
	int old_servo_state;
	//uint16_t reg_data = 0xeeee;
	uint8_t * log_buffer = NULL;
	int i;
	if(reg_offset + size/4 -1 > 0xFF)
	{
		pr_err("offset 0x%x + size %d, target offset reg address 0x%x too large, max offset value is 0xFF!\n",
		reg_offset,
		size,
		reg_offset + size/4);
		return -1;
	}

	onsemi_get_servo_state(ctrl,&old_servo_state);

	onsemi_servo_go_off(ctrl);


	onsemi_write_byte(ctrl,0x000F,size/4);//data read 256 bytes, 64, one unit is 4 bytes
	onsemi_write_word(ctrl,0x0010,reg_offset);//data read start address offset,0x7400-0x74ff
	onsemi_write_byte(ctrl,0x000E,0x04);//Read command set

	//onsemi_read_byte(ctrl,0x000E,&reg_data);//DFLSCMD READ
	rc = onsemi_wait_value_with_timeout(ctrl,0x000E,0xff,0x14,10000,__func__);

	if(rc == 0)
	{
		pr_info("Read User Data Complete!\n");
		onsemi_read_seq_bytes(ctrl,0x0100,data,size);

		log_buffer = kzalloc(sizeof(uint8_t)*size*8,GFP_KERNEL);
		if (!log_buffer)
		{
			pr_err("no memory!\n");
			return -ENOMEM;
		}
		if(format_hex_string(log_buffer,sizeof(uint8_t)*size*8,data,size)!= -1)
		{
			pr_info("Read User Data as follows, size %d\n",size);
			//pr_info("%s",log_buffer);//can not be too large ....
			for(i=0;i<size;i++)
			{
				pr_info("reg 0x%04x, val 0x%02x\n",reg_offset+i,data[i]);
			}
		}

		kfree(log_buffer);
		rc = 0;

	}
	else
	{
		pr_err("Read user data command failed\n");
		rc = -2;
	}

	onsemi_restore_servo_state(ctrl,old_servo_state);

	return rc;
}


static uint32_t get_fw_revision_from_code_data(uint8_t* fw_code_data)
{
	uint32_t revision;
	revision = ((uint32_t)(fw_code_data[0x6FF7] <<24)|
			 (uint32_t)(fw_code_data[0x6FF6] <<16)|
			 (uint32_t)(fw_code_data[0x6FF5] <<8)|
			 (uint32_t)(fw_code_data[0x6FF4] )
			);
	return revision;
}
#if 0
static uint16_t get_check_sum(uint8_t* data, uint16_t size)
{
	uint16_t i;
	uint16_t check_sum;

	check_sum = 0;
	for(i=0;i<size;i++)
	{
		check_sum+=data[i];
	}
	pr_info("check sum of char type is 0x%x -> %hu, size %hu\n",check_sum,check_sum,size);
	return check_sum;
}
#endif
static uint16_t get_check_sum_short(uint16_t* data, uint16_t size)
{
	uint16_t i;
	uint16_t check_sum;

	check_sum = 0;
	for(i=0;i<size/2;i++)
	{
		check_sum+=data[i];
	}
	pr_info("check sum of short type is 0x%x -> %hu, size %hu\n",check_sum,check_sum,size);
	return check_sum;
}

int onsemi_update_fw(struct msm_ois_ctrl_t *ctrl, const char *file_path)
{
	#define SEND_SIZE 128
	int rc;
	uint64_t fw_file_size;
	uint8_t* fw_code_data;
	uint32_t fw_revision;
	uint32_t reg_fw_revision;
	int i;
	uint16_t reg_data = 0xeeee;
	uint16_t check_sum;
	uint8_t  send_data[4];

#ifdef DUMP_CHECK
	char dumpFileName[64];
	uint8_t  send_block_data[SEND_SIZE];
#endif
	uint32_t checksum_val = 0xeeee;
	if(!onsemi_is_idle_state(ctrl))
	{
		return -1;
	}

	fw_file_size = get_file_size(file_path);

	if(fw_file_size < 0x6FF7+1)
	{
		pr_err("fw file size %lld is less than 0x6FF8, not valid firmware!\n",fw_file_size);
		return -2;
	}

	if(fw_file_size != 28672)
	{
		pr_err("fw file size %lld is not 28672!\n",fw_file_size);
		return -9;
	}

	fw_code_data = kzalloc(sizeof(uint8_t)*fw_file_size,GFP_KERNEL);
	if (!fw_code_data)
	{
		pr_err("no memory!\n");
		return -ENOMEM;
	}

	if(read_file_into_buffer(file_path,fw_code_data,fw_file_size) < 0)
	{
		rc = -3;
		pr_err("read fw data to buffer failed!\n");
		goto Error;
	}

	fw_revision = get_fw_revision_from_code_data(fw_code_data);

	onsemi_read_dword(ctrl,0x00FC,&reg_fw_revision);

#if 0
	if(reg_fw_revision != 15197 && reg_fw_revision != 14819)
	{
		pr_err("device FW revision %d not valid, can not update!\n",
			reg_fw_revision
		);
		rc = -10;
		goto Error;
	}
#endif
	if(fw_revision != 15456)
	{
		pr_err("File %s FW revision %d not valid, can not update!\n",
			file_path,fw_revision
		);
		rc = -11;
		goto Error;
	}

	if(fw_revision < reg_fw_revision)
	{
		pr_err("Warning! FW file revision %d less than device FW revision %d\n",
			fw_revision,reg_fw_revision
		);
		rc = -8;
		goto Error;
	}

	pr_info("Going update FW, dev_fw_rev 0x%x -> %d, bin_fw_rev 0x%x -> %d, bin file size %lld\n",
			reg_fw_revision,reg_fw_revision,
			fw_revision,fw_revision,
			fw_file_size
	);
#if 1
	// 0 1 1 1  x x x  1
	/*
	0: 64bytes
	1: 128bytes
	2: 256bytes
	3: 8bytes
	4: 16bytes
	5: 32bytes
	*/
	onsemi_write_byte(ctrl,0x000C,0x73);//Update block size=7, FWUP_DSIZE=128bytes, FWUPEN=Start

	delay_ms(55);//Wait for FlashROM erase 7 blocks

	/* Write FW Code Data */
	for(i=0;i<fw_file_size/SEND_SIZE;i++)
	{
		rc = onsemi_write_seq_bytes(ctrl,0x0100,fw_code_data+i*SEND_SIZE,SEND_SIZE);
#ifdef DUMP_CHECK
		snprintf(dumpFileName,sizeof(dumpFileName),"/sdcard/ois_fw/send/%d",i);
		sysfs_write_byte_seq(dumpFileName,fw_code_data+i*SEND_SIZE,SEND_SIZE);

		rc = onsemi_read_seq_bytes(ctrl,0x0100,send_block_data,SEND_SIZE);
		snprintf(dumpFileName,sizeof(dumpFileName),"/sdcard/ois_fw/rec/%d",i);
		sysfs_write_byte_seq(dumpFileName,send_block_data,SEND_SIZE);
#endif
		if(rc < 0)
		{
			pr_err("send code data failed! rc = %d\n",rc);
			rc = -4;
			goto Error;
		}
		delay_ms(5);//Wait for FlashROM writing 64bytes*2
	}

	/* Write Error Status Check*/
	onsemi_read_word(ctrl,0x0006,&reg_data);
	if(reg_data == 0x0000)
	{
		check_sum = get_check_sum_short((uint16_t*)fw_code_data,fw_file_size);
		//get_check_sum(fw_code_data,fw_file_size);
		send_data[0] = (check_sum & 0x00FF);
		send_data[1] = (check_sum & 0xFF00) >> 8;
		send_data[2] = 0;
		send_data[3] = 0x80; //Self Reset Request

		onsemi_write_seq_bytes(ctrl,0x0008,send_data,4);

		onsemi_read_dword(ctrl,0x0008,&checksum_val);
		pr_info("0x0008 val is 0x%x\n",checksum_val);
		delay_ms(190);//Wait Self Reset + OIS Data Section init, 20+170 ms

		onsemi_read_word(ctrl,0x0006,&reg_data);//Error Status read

		if(reg_data == 0x0000)
		{
			onsemi_read_dword(ctrl,0x00FC,&reg_fw_revision);
			if(reg_fw_revision == fw_revision)
			{
				pr_info("FW revision 0x%x -> %d update Succeeded!\n",
					reg_fw_revision,
					reg_fw_revision
					);
			}
			else
			{
				pr_err("FW revision (code fw 0x%x reg fw 0x%x) update failed!\n",
					fw_revision,
					reg_fw_revision
					);
				rc = -5;
				goto Error;
			}
		}
		else
		{
			pr_err("FW Update Error! reg 0x0006 val 0x%x\n",reg_data);
			rc = -6;
			goto Error;
		}
	}
	else
	{
		pr_err("FW Code Write to Flash ROM Error! reg 0x0006 val 0x%x\n",reg_data);
		rc = -7;
		goto Error;
	}
#endif
	kfree(fw_code_data);
	return 0;
Error:
	kfree(fw_code_data);
	return rc;
}

//ASUS_BSP Lucien +++: Replace update ois fw after reading vcm data
int onsemi_update_fw_from_eeprom(struct msm_ois_ctrl_t *ctrl, uint32_t mode, uint8_t vcm)
{
	int rc = 0;
	uint32_t fw_version = 0;
	pr_info("%s: E\n",__func__);
	rc = onsemi_read_dword(ctrl,0x8000,&fw_version);
	if(rc < 0)
	{
		pr_err("%s: Read firmware version failed\n", __func__);
		return rc;
	}

	rc = msm_ois_check_update_fw(ctrl, mode, fw_version, vcm);
	if(rc < 0) pr_err("%s: onsemi_update_fw_from_eeprom failed\n", __func__);

	pr_info("%s: X\n",__func__);

	return 0;
}
//ASUS_BSP Lucien ---: Replace update ois fw after reading vcm data

typedef struct
{
	uint16_t reg_addr;
	uint32_t reg_val;
	uint8_t  length;
}update_command_t;

int onsemi_update_parameter_for_updating_fw(struct msm_ois_ctrl_t *ctrl)
{
	update_command_t commands[18] =
	{
		{0x0230,0x50,1},
		{0x0231,0x14,1},
		{0x0232,0x46,1},
		{0x0233,0x1E,1},
		{0x0318,0x3EA8F5C3,4},
		{0x031C,0x3F7F3B64,4},
		{0x0348,0x3F28F5C3,4},
		{0x034C,0x3F800000,4},
		{0x0378,0x3F800000,4},
		{0x0385,0x64,1},
		{0x0387,0x64,1},
		{0x03A8,0x3F800000,4},
		{0x03B5,0x64,1},
		{0x03B7,0x64,1},
		{0x0454,0x39AEC2CC,4},
		{0x0458,0x3F7F84B3,4},
		{0x045C,0x3AF6999E,4},
		{0x0460,0x000000FA,4}
	};
	int i;
	int rc = 0;
	for(i=0;i<18;i++)
	{
		if(commands[i].length == 1)
		{
			pr_info("Write 0x%hX to reg 0x%04X for updating FW\n",(uint16_t)commands[i].reg_val,commands[i].reg_addr);
			rc = onsemi_write_byte(ctrl,commands[i].reg_addr,(uint16_t)commands[i].reg_val);
		}
		else if(commands[i].length == 4)
		{
			pr_info("Write 0x%X to reg 0x%04X for updating FW\n",commands[i].reg_val,commands[i].reg_addr);
			rc = onsemi_write_dword(ctrl,commands[i].reg_addr,commands[i].reg_val);
		}
	}
	return rc;

}

//********************************************************************************
// Function Name 	: F40_IOWrite32A
//********************************************************************************
void F40_IORead32A( struct msm_ois_ctrl_t *ctrl, uint32_t IOadrs, uint32_t *IOdata )
{
	RamWrite32A( ctrl, F40_IO_ADR_ACCESS, IOadrs ) ;
	RamRead32A ( ctrl, F40_IO_DAT_ACCESS, IOdata ) ;
}

//********************************************************************************
// Function Name 	: F40_IOWrite32A
//********************************************************************************
void F40_IOWrite32A( struct msm_ois_ctrl_t *ctrl, uint32_t IOadrs, uint32_t IOdata )
{
	RamWrite32A( ctrl, F40_IO_ADR_ACCESS, IOadrs ) ;
	RamWrite32A( ctrl, F40_IO_DAT_ACCESS, IOdata ) ;
}

//********************************************************************************
// Function Name 	: WPB level read
//********************************************************************************
uint8_t F40_ReadWPB( struct msm_ois_ctrl_t *ctrl )
{
	uint32_t UlReadVal, UlCnt=0;

	do{
		F40_IORead32A( ctrl, FLASHROM_F40_WPB, &UlReadVal );
		if( (UlReadVal & 0x00000004) != 0 )	return ( 1 ) ;
		WitTim( 1 );
	}while ( UlCnt++ < 10 );
	return ( 0 );
}

//********************************************************************************
// Function Name 	: F40_UnlockCodeSet
//********************************************************************************
uint8_t F40_UnlockCodeSet( struct msm_ois_ctrl_t *ctrl )
{
	uint32_t UlReadVal ;

	//WPBCtrl( WPB_OFF ) ;
	if ( F40_ReadWPB(ctrl) != 1 )
		return ( 5 ) ;

	F40_IOWrite32A( ctrl, FLASHROM_F40_UNLK_CODE1,	0xAAAAAAAA ) ;
	F40_IOWrite32A( ctrl, FLASHROM_F40_UNLK_CODE2,	0x55555555 ) ;
	F40_IOWrite32A( ctrl, FLASHROM_F40_RSTB_FLA,	       0x00000001 ) ;
	F40_IOWrite32A( ctrl, FLASHROM_F40_CLK_FLAON,	0x00000010 ) ;
	F40_IOWrite32A( ctrl, FLASHROM_F40_UNLK_CODE3,	0x0000ACD5 ) ;
	F40_IOWrite32A( ctrl, FLASHROM_F40_WPB,			0x00000001 ) ;
	RamRead32A(  ctrl, F40_IO_DAT_ACCESS, &UlReadVal ) ;

	if ( (UlReadVal & 0x00000007) != 7 )
		return( 1 ) ;

	return( 0 ) ;
}

//********************************************************************************
// Function Name 	: F40_UnlockCodeClear
//********************************************************************************
uint8_t F40_UnlockCodeClear(struct msm_ois_ctrl_t *ctrl)
{
	uint32_t UlReadVal ;

	F40_IOWrite32A( ctrl, FLASHROM_F40_WPB, 0x00000010 ) ;
	RamRead32A( ctrl, F40_IO_DAT_ACCESS,	&UlReadVal ) ;

	if( (UlReadVal & 0x00000080) != 0 )
		return( 3 ) ;

	//WPBCtrl( WPB_ON ) ;

	return( 0 ) ;
}

//********************************************************************************
// Function Name 	: F40_FlashBlockErase
//********************************************************************************
uint8_t F40_FlashBlockErase( struct msm_ois_ctrl_t *ctrl, uint32_t SetAddress )
{
	uint32_t	UlReadVal, UlCnt ;
	uint8_t	ans	= 0 ;

	if( SetAddress & 0x00010000 )
		return( 9 );

	ans	= F40_UnlockCodeSet(ctrl) ;
	if( ans != 0 )
		return( ans ) ;

	F40_IOWrite32A( ctrl, FLASHROM_F40_ADR,	(SetAddress & 0xFFFFFE00) ) ;
	F40_IOWrite32A( ctrl, FLASHROM_F40_CMD,	0x00000006 ) ;

	WitTim( 5 ) ;

	UlCnt	= 0 ;
	do {
		if( UlCnt++ > 100 ) {
			ans = 2 ;
			break ;
		}

		F40_IORead32A( ctrl, FLASHROM_F40_INT, &UlReadVal ) ;
	} while( ( UlReadVal & 0x00000080 ) != 0 ) ;

	F40_UnlockCodeClear(ctrl) ;

	return( ans ) ;
}

//********************************************************************************
// Function Name 	: F40_FlashBurstWrite
//********************************************************************************
uint8_t F40_FlashBurstWrite( struct msm_ois_ctrl_t *ctrl, const uint8_t *NcDataVal,
uint32_t NcDataLength, uint32_t ScNvrMan )
{
	uint32_t	i, j, UlCnt ;
	uint8_t	data[163] ;  // ComdH + CmdL + Length + Data[40]
	uint32_t	UlReadVal ;
	uint8_t	UcOddEvn ;
	uint8_t	Remainder ;

	data[0] = 0xF0 ;  // Command High
	data[1] = 0x08 ;  // Command Low
	data[2] = BURST_LENGTH ;

	for( i = 0 ; i < (NcDataLength / BURST_LENGTH) ; i++ ) {
		if( i!= 0 && !(i % 100) ) msleep(0);
		UlCnt = 3 ;

		UcOddEvn =i % 2 ;
		data[1] = 0x08 + UcOddEvn ;

		for( j = 0 ; j < BURST_LENGTH; j++ )
			data[UlCnt++] = *NcDataVal++ ;

		CntWrt( ctrl, data, BURST_LENGTH + 3 ) ;
		RamWrite32A( ctrl, 0xF00A ,(UINT_32) ( ( BURST_LENGTH / 5 ) * i + ScNvrMan) ) ;  // set Flash write address
		RamWrite32A( ctrl, 0xF00B ,(UINT_32) (BURST_LENGTH / 5) ) ;                                // Word Address
		RamWrite32A( ctrl, 0xF00C , 4 + 4 * UcOddEvn ) ;                                                   // set write operation
	}

	Remainder = NcDataLength % BURST_LENGTH ;
	if( Remainder != 0 ) {
		data[2] = Remainder ;
		UlCnt = 3 ;
		UcOddEvn =i % 2 ;
		data[1] = 0x08 + UcOddEvn ;

		for( j = 0 ; j < Remainder; j++ )
			data[UlCnt++] = *NcDataVal++ ;

		CntWrt( ctrl, data, BURST_LENGTH + 3 ) ;
		RamWrite32A( ctrl, 0xF00A ,(UINT_32) ( ( BURST_LENGTH / 5 ) * i + ScNvrMan) ) ;  // set Flash write address
		RamWrite32A( ctrl, 0xF00B ,(UINT_32) (Remainder /5) ) ;                                         // Word Address
		RamWrite32A( ctrl, 0xF00C , 4 + 4 * UcOddEvn ) ;                                                    // set write operation
	}

	UlCnt = 0 ;
	do {
		if( UlCnt++ > 100 )
			return ( 1 );

		RamRead32A( ctrl, 0xF00C, &UlReadVal ) ;
	} while ( UlReadVal != 0 ) ;

	return( 0 );
}


//********************************************************************************
// Function Name 	: F40_FlashSectorRead
//********************************************************************************
void F40_FlashSectorRead( struct msm_ois_ctrl_t *ctrl, uint32_t UlAddress, uint8_t *PucData )
{
	uint8_t	UcIndex, UcNum ;
	uint32_t	UcReadDat ;

	F40_IOWrite32A( ctrl, FLASHROM_F40_ADR, ( UlAddress & 0xFFFFFFC0 ) ) ;
	F40_IOWrite32A( ctrl, FLASHROM_F40_ACSCNT, 63 ) ;
	UcNum	= 64 ;

	F40_IOWrite32A( ctrl, FLASHROM_F40_CMD, 0x00000001 ) ;  // Read Start

	for( UcIndex = 0 ; UcIndex < UcNum ; UcIndex++ ) {
		RamWrite32A( ctrl, F40_IO_ADR_ACCESS, FLASHROM_F40_RDATH ) ;
		RamRead32A(  ctrl, F40_IO_DAT_ACCESS, &UcReadDat ) ;
		*PucData++		= UcReadDat & 0x000000FF ;
		RamWrite32A( ctrl, F40_IO_ADR_ACCESS, FLASHROM_F40_RDATL ) ;
		RamRead32A(  ctrl, F40_IO_DAT_ACCESS, &UcReadDat ) ;
		*PucData++	= UcReadDat & 0xFF000000 >> 24;
		*PucData++	= UcReadDat & 0x00FF0000 >> 16;
		*PucData++	= UcReadDat & 0x0000FF00 >> 8;
		*PucData++	= UcReadDat & 0x000000FF ;
	}
}

//********************************************************************************
// Function Name 	: F40_CalcChecksum
//********************************************************************************
void F40_CalcChecksum( const uint8_t *pData, uint32_t len, uint32_t *pSumH, uint32_t *pSumL )
{
	uint64_t sum = 0 ;
	uint64_t dat ;
	uint16_t i ;

	for( i = 0; i < len / 5; i++ ) {
		sum  += ((uint64_t)*pData++) << 32;
		dat  = ((uint64_t)*pData++) << 24;
		dat += ((uint64_t)*pData++) << 16;
		dat += ((uint64_t)*pData++) << 8;
		dat += ((uint64_t)*pData++) ;
		sum += dat ;
	}

	*pSumH = (uint32_t)(sum >> 32) ;
	*pSumL = (uint32_t)(sum & 0xFFFFFFFF) ;
}

//********************************************************************************
// Function Name 	: F40_CalcBlockChksum
//********************************************************************************
void F40_CalcBlockChksum( struct msm_ois_ctrl_t *ctrl, uint8_t num, uint32_t *pSumH, uint32_t *pSumL )
{
	uint8_t	SectorData[SECTOR_SIZE] ;
	uint32_t	top ;
	uint16_t	sec ;
	uint64_t	sum = 0 ;
	uint32_t	datH, datL ;

	top = num * BLOCK_UNIT ;

	for( sec = 0; sec < (BLOCK_BYTE / SECTOR_SIZE); sec++ ) {
		F40_FlashSectorRead( ctrl,  top + sec * 64, SectorData ) ;

		F40_CalcChecksum( SectorData, SECTOR_SIZE, &datH, &datL ) ;
		sum += ((uint64_t)datH << 32) + datL ;
	}

	*pSumH = (uint32_t)(sum >> 32);
	*pSumL = (uint32_t)(sum & 0xFFFFFFFF);
}


//********************************************************************************
// Function Name 	: F40_FlashDownload
//********************************************************************************
uint8_t F40_FlashDownload( struct msm_ois_ctrl_t *ctrl, uint8_t mode, uint8_t ModuleVendor, uint8_t ActVer )
{
	DOWNLOAD_TBL* ptr ;

	ptr = ( DOWNLOAD_TBL * )DTbl ;
	do {
		if( ptr->Index == ( ((UINT_16)ModuleVendor<<8) + ActVer) ) {
			return F40_FlashUpdate( ctrl, mode, ptr );
		}
		ptr++ ;
	} while (ptr->Index != 0xFFFF ) ;

	return 0xF0 ;
}

//********************************************************************************
// Function Name 	: F40_FlashUpdate
//********************************************************************************
uint8_t F40_FlashUpdate( struct msm_ois_ctrl_t *ctrl, uint8_t flag, DOWNLOAD_TBL* ptr )
{
	uint32_t	SiWrkVl0 ,SiWrkVl1 ;
	uint32_t	SiAdrVal ;
	const uint8_t *NcDatVal ;
	uint32_t	UlReadVal, UlCnt ;
	uint8_t	ans, i ;
	uint16_t	UsChkBlocks ;
	uint8_t	UserMagicCode[ ptr->SizeMagicCode ] ;

//--------------------------------------------------------------------------------
// 0.
//--------------------------------------------------------------------------------
	F40_IOWrite32A( ctrl, SYSDSP_REMAP, 0x00001440 ) ;
	WitTim( 25 ) ;
	F40_IORead32A( ctrl, SYSDSP_SOFTRES,	&SiWrkVl0 ) ;
	SiWrkVl0	&= 0xFFFFEFFF ;
	F40_IOWrite32A( ctrl, SYSDSP_SOFTRES, SiWrkVl0 ) ;
	RamWrite32A( ctrl, 0xF006, 0x00000000 ) ;
	F40_IOWrite32A( ctrl, SYSDSP_DSPDIV, 0x00000001 ) ;
	RamWrite32A( ctrl, 0x0344, 0x00000014 ) ;
	SiAdrVal =0x00100000;

	for( UlCnt = 0 ;UlCnt < 25 ; UlCnt++ ) {
		RamWrite32A( ctrl, 0x0340, SiAdrVal ) ;
		SiAdrVal += 0x00000008 ;
		RamWrite32A( ctrl, 0x0348, UlPmemCodeF40[ UlCnt*5   ] ) ;
		RamWrite32A( ctrl, 0x034C, UlPmemCodeF40[ UlCnt*5+1 ] ) ;
		RamWrite32A( ctrl, 0x0350, UlPmemCodeF40[ UlCnt*5+2 ] ) ;
		RamWrite32A( ctrl, 0x0354, UlPmemCodeF40[ UlCnt*5+3 ] ) ;
		RamWrite32A( ctrl, 0x0358, UlPmemCodeF40[ UlCnt*5+4 ] ) ;
		RamWrite32A( ctrl, 0x033c, 0x00000001 ) ;
	}
	for(UlCnt = 0 ;UlCnt < 9 ; UlCnt++ ){
		CntWrt( ctrl, (char*)&UpData_CommandFromTable[ UlCnt*6 ], 0x00000006 ) ;
	}

//--------------------------------------------------------------------------------
// 1.
//--------------------------------------------------------------------------------
	if( flag ) {
		ans = F40_UnlockCodeSet(ctrl) ;
		if ( ans != 0 )
			return( ans ) ;

		F40_IOWrite32A( ctrl, FLASHROM_F40_ADR, 0x00000000 ) ;
		F40_IOWrite32A( ctrl, FLASHROM_F40_CMD, 0x00000005 ) ;
		WitTim( 13 ) ;
		UlCnt=0;
		do {
			if( UlCnt++ > 100 ) {
				ans=0x10 ;
				break ;
			}
			F40_IORead32A( ctrl, FLASHROM_F40_INT, &UlReadVal ) ;
		}while ( (UlReadVal & 0x00000080) != 0 ) ;

	} else {
		for( i = 0 ; i < ERASE_BLOCKS ; i++ ) {
			ans	= F40_FlashBlockErase( ctrl, i * BLOCK_UNIT ) ;
			if( ans != 0 ) {
				return( ans ) ;
			}
		}
		ans = F40_UnlockCodeSet(ctrl) ;
		if ( ans != 0 )
			return( ans );
	}
//--------------------------------------------------------------------------------
// 2.
//--------------------------------------------------------------------------------
	F40_IOWrite32A( ctrl, FLASHROM_F40_ADR, 0x00010000 ) ;
	F40_IOWrite32A( ctrl, FLASHROM_F40_CMD, 0x00000004 ) ;
	WitTim( 5 ) ;
	UlCnt=0;
	do {
		if( UlCnt++ > 100 ) {
			ans = 0x10 ;
			break ;
		}
		F40_IORead32A( ctrl, FLASHROM_F40_INT, &UlReadVal ) ;
	} while ( (UlReadVal & 0x00000080) != 0 ) ;

//--------------------------------------------------------------------------------
// 3.
//--------------------------------------------------------------------------------
	F40_FlashBurstWrite( ctrl, ptr->FromCode, ptr->SizeFromCode, 0 ) ;

	ans |= F40_UnlockCodeClear(ctrl) ;
	if ( ans != 0 )
		return( ans ) ;

//--------------------------------------------------------------------------------
// 4.
//--------------------------------------------------------------------------------
	UsChkBlocks = ( ptr->SizeFromCode / 160 ) + 1 ;
	RamWrite32A( ctrl, 0xF00A, 0x00000000 ) ;
	RamWrite32A( ctrl, 0xF00B, UsChkBlocks ) ;
	RamWrite32A( ctrl, 0xF00C, 0x00000100 ) ;

	NcDatVal = ptr->FromCode;
	SiWrkVl0 = 0;
	for( UlCnt = 0; UlCnt < ptr->SizeFromCode; UlCnt++ ) {
		SiWrkVl0 += *NcDatVal++ ;
	}
	UsChkBlocks *= 160  ;
	for( ; UlCnt < UsChkBlocks ; UlCnt++ ) {
		SiWrkVl0 += 0xFF ;
	}

	UlCnt=0;
	do {
		if( UlCnt++ > 100 )
			return ( 6 ) ;

		RamRead32A( ctrl, 0xF00C, &UlReadVal ) ;
	} while( UlReadVal != 0 );

	RamRead32A( ctrl, 0xF00D, &SiWrkVl1 ) ;

	if( SiWrkVl0 != SiWrkVl1 )
		return( 0x20 );

//--------------------------------------------------------------------------------
// X.
//--------------------------------------------------------------------------------

	if ( !flag ) {
		uint32_t sumH, sumL;
		uint16_t Idx;

		// if you can use memcpy(), modify code.
		for( UlCnt = 0; UlCnt < ptr->SizeMagicCode; UlCnt++ ) {
			UserMagicCode[ UlCnt ] = ptr->MagicCode[ UlCnt ] ;
		}

		for( UlCnt = 0; UlCnt < USER_RESERVE; UlCnt++ ) {
			F40_CalcBlockChksum( ctrl, ERASE_BLOCKS + UlCnt, &sumH, &sumL ) ;
			Idx =  (ERASE_BLOCKS + UlCnt) * 2 * 5 + 1 + 40 ;
			NcDatVal = (UINT_8 *)&sumH ;

#ifdef _BIG_ENDIAN_
			// for BIG ENDIAN SYSTEM
			UserMagicCode[ Idx++ ] = *NcDatVal++ ;
			UserMagicCode[ Idx++ ] = *NcDatVal++ ;
			UserMagicCode[ Idx++ ] = *NcDatVal++ ;
			UserMagicCode[ Idx++ ] = *NcDatVal++ ;
			Idx++;
			NcDatVal = (UINT_8 *)&sumL;
			UserMagicCode[ Idx++ ] = *NcDatVal++ ;
			UserMagicCode[ Idx++ ] = *NcDatVal++ ;
			UserMagicCode[ Idx++ ] = *NcDatVal++ ;
			UserMagicCode[ Idx++ ] = *NcDatVal++ ;
#else
			// for LITTLE ENDIAN SYSTEM
			UserMagicCode[ Idx+3 ] = *NcDatVal++ ;
			UserMagicCode[ Idx+2 ] = *NcDatVal++ ;
			UserMagicCode[ Idx+1 ] = *NcDatVal++ ;
			UserMagicCode[ Idx+0 ] = *NcDatVal++ ;
			Idx+=5;
			NcDatVal = (UINT_8 *)&sumL;
			UserMagicCode[ Idx+3 ] = *NcDatVal++ ;
			UserMagicCode[ Idx+2 ] = *NcDatVal++ ;
			UserMagicCode[ Idx+1 ] = *NcDatVal++ ;
			UserMagicCode[ Idx+0 ] = *NcDatVal++ ;
#endif
		}
		NcDatVal = UserMagicCode ;

	} else {
		NcDatVal = ptr->MagicCode ;
	}

//--------------------------------------------------------------------------------
// 5.
//--------------------------------------------------------------------------------
	ans = F40_UnlockCodeSet(ctrl) ;
	if ( ans != 0 )
		return( ans ) ;

	F40_FlashBurstWrite( ctrl, NcDatVal, ptr->SizeMagicCode, 0x00010000 );
	F40_UnlockCodeClear(ctrl);

//--------------------------------------------------------------------------------
// 6.
//--------------------------------------------------------------------------------
	RamWrite32A( ctrl, 0xF00A, 0x00010000 ) ;
	RamWrite32A( ctrl, 0xF00B, 0x00000002 ) ;
	RamWrite32A( ctrl, 0xF00C, 0x00000100 ) ;

	SiWrkVl0 = 0;
	for( UlCnt = 0; UlCnt < ptr->SizeMagicCode; UlCnt++ ) {
		SiWrkVl0 += *NcDatVal++ ;
	}
	for( ; UlCnt < 320; UlCnt++ ) {
		SiWrkVl0 += 0xFF ;
	}

	UlCnt=0 ;
	do {
		if( UlCnt++ > 100 )
			return( 6 ) ;

		RamRead32A( ctrl, 0xF00C, &UlReadVal ) ;
	} while( UlReadVal != 0 ) ;
	RamRead32A( ctrl, 0xF00D,	&SiWrkVl1 ) ;

	if(SiWrkVl0 != SiWrkVl1 )
		return( 0x30 );

	F40_IOWrite32A( ctrl, SYSDSP_REMAP, 0x00001000 ) ;
	return( 0 );
}

//********************************************************************************
// Function Name 	: F40_ReadCalData
//********************************************************************************

void F40_ReadCalData( struct msm_ois_ctrl_t *ctrl, uint32_t * BufDat, uint32_t * ChkSum )
{
	uint16_t	UsSize = 0, UsNum;

	*ChkSum = 0;

	do{
		// Count
		F40_IOWrite32A( ctrl, FLASHROM_F40_ACSCNT, (FLASH_ACCESS_SIZE-1) ) ;

		// NVR2 Addres Set
		F40_IOWrite32A( ctrl, FLASHROM_F40_ADR, 0x00010040 + UsSize ) ;		// set NVR2 area
		// Read Start
		F40_IOWrite32A( ctrl, FLASHROM_F40_CMD, 1 ) ;  						// Read Start

		RamWrite32A( ctrl, F40_IO_ADR_ACCESS , FLASHROM_F40_RDATL ) ;		// RDATL data

		for( UsNum = 0; UsNum < FLASH_ACCESS_SIZE; UsNum++ )
		{
			RamRead32A(  ctrl, F40_IO_DAT_ACCESS , &(BufDat[ UsSize ]) ) ;
			*ChkSum += BufDat[ UsSize++ ];
		}
	}while (UsSize < 64);	// 64*5 = 320 : NVR sector size
}

//********************************************************************************
// Function Name 	: F40_GyroReCalib
//********************************************************************************
uint8_t	F40_GyroReCalib( struct msm_ois_ctrl_t *ctrl, stReCalib * pReCalib )
{
	uint8_t	UcSndDat ;
	uint32_t	UlRcvDat ;
	uint32_t	UlGofX, UlGofY ;
	uint32_t	UiChkSum ;
	uint32_t	UlStCnt = 0;

	F40_ReadCalData( ctrl, UlBufDat, &UiChkSum );

	RamWrite32A( ctrl, 0xF014 , 0x00000000 ) ;  //High level cmd: do calibration

	do {
		UcSndDat = F40_RdStatus(ctrl, 1);
	} while (UcSndDat != 0 && (UlStCnt++ < CNT100MS ));

	RamRead32A( ctrl, 0xF014 , &UlRcvDat ) ;
	UcSndDat = (unsigned char)(UlRcvDat >> 24);

	if( UlBufDat[ 49 ] == 0xFFFFFFFF )
		pReCalib->SsFctryOffX = (UlBufDat[ 19 ] >> 16) ;  //GYRO_OFFSET_X
	else
		pReCalib->SsFctryOffX = (UlBufDat[ 49 ] >> 16) ;  //GYRO_FCTRY_OFST_X

	if( UlBufDat[ 50 ] == 0xFFFFFFFF )
		pReCalib->SsFctryOffY = (UlBufDat[ 20 ] >> 16) ;  //GYRO_OFFSET_Y
	else
		pReCalib->SsFctryOffY = (UlBufDat[ 50 ] >> 16) ;  //GYRO_FCTRY_OFST_Y

	RamRead32A(  ctrl, 0x0278 , &UlGofX ) ;  //GYRO_RAM_GXOFFZ
	RamRead32A(  ctrl, 0x027C , &UlGofY ) ;  //GYRO_RAM_GYOFFZ

	pReCalib->SsRecalOffX = (UlGofX >> 16) ;
	pReCalib->SsRecalOffY = (UlGofY >> 16) ;
	pReCalib->SsDiffX = pReCalib->SsFctryOffX - pReCalib->SsRecalOffX ;
	pReCalib->SsDiffY = pReCalib->SsFctryOffY - pReCalib->SsRecalOffY ;

	return( UcSndDat );
}
//********************************************************************************
// Function Name 	: F40_EraseCalData
//********************************************************************************
uint8_t F40_EraseCalData( struct msm_ois_ctrl_t *ctrl )
{
	uint32_t	UlReadVal, UlCnt;
	uint8_t ans = 0;

	// Flash write·Ç³Æ
	ans = F40_UnlockCodeSet(ctrl);
	if ( ans != 0 ) return (ans);								// Unlock Code Set

	// set NVR2 area
	F40_IOWrite32A( ctrl, FLASHROM_F40_ADR, 0x00010040 ) ;
	// Sector Erase Start
	F40_IOWrite32A( ctrl, FLASHROM_F40_CMD, 4	/* SECTOR ERASE */ ) ;

	WitTim( 5 ) ;
	UlCnt=0;
	do{
		if( UlCnt++ > 100 ){	ans = 2;	break;	} ;
		F40_IORead32A( ctrl, FLASHROM_F40_INT, &UlReadVal );
	}while ( (UlReadVal & 0x00000080) != 0 );

	ans = F40_UnlockCodeClear(ctrl);									// Unlock Code Clear

	return(ans);
}

//********************************************************************************
// Function Name 	: F40_WrGyroOffsetData
//********************************************************************************
uint8_t	F40_WrGyroOffsetData( struct msm_ois_ctrl_t *ctrl )
{
	uint32_t	UlFctryX, UlFctryY;
	uint32_t	UlCurrX, UlCurrY;
	uint32_t	UlGofX, UlGofY;
	uint32_t	UiChkSum1,	UiChkSum2 ;
	uint32_t	UlSrvStat,	UlOisStat ;
	uint8_t	ans;
	uint32_t	UlStCnt = 0;
	uint8_t	UcSndDat ;


	RamRead32A( ctrl, 0xF010 , &UlSrvStat ) ;
	RamRead32A( ctrl, 0xF012 , &UlOisStat ) ;
	RamWrite32A( ctrl, 0xF010 , 0x00000000 ) ;
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
	F40_ReadCalData( ctrl, UlBufDat, &UiChkSum2 );
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
	ans = F40_EraseCalData(ctrl);
	if ( ans == 0 ){
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
		RamRead32A(  ctrl, 0x0278 , &UlGofX ) ;
		RamRead32A(  ctrl, 0x027C , &UlGofY ) ;

		UlCurrX		= UlBufDat[ 19 ] ;
		UlCurrY		= UlBufDat[ 20 ] ;
		UlFctryX	= UlBufDat[ 49 ] ;
		UlFctryY	= UlBufDat[ 50 ] ;

		if( UlFctryX == 0xFFFFFFFF )
			UlBufDat[ 49 ] = UlCurrX ;

		if( UlFctryY == 0xFFFFFFFF )
			UlBufDat[ 50 ] = UlCurrY ;

		UlBufDat[ 19 ] = UlGofX ;
		UlBufDat[ 20 ] = UlGofY ;

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
		F40_WriteCalData( ctrl, UlBufDat, &UiChkSum1 );	
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
		F40_ReadCalData( ctrl, UlBufDat, &UiChkSum2 );	

		if(UiChkSum1 != UiChkSum2 ){
			ans = 0x10;
		}
	}
//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
	if( !UlSrvStat ) {
		RamWrite32A( ctrl, 0xF010 , 0x00000000 ) ;
	} else if( UlSrvStat == 3 ) {
		RamWrite32A( ctrl, 0xF010 , 0x00000003 ) ;
	} else {
		RamWrite32A( ctrl, 0xF010 , UlSrvStat ) ;
	}
	do {
		UcSndDat = F40_RdStatus(ctrl, 1);
	} while( UcSndDat == FAILURE && (UlStCnt++ < CNT200MS ));

	if( UlOisStat != 0) {
		RamWrite32A( ctrl, 0xF012 , 0x00000001 ) ;
		UlStCnt = 0;
		UcSndDat = 0;
		while( UcSndDat && ( UlStCnt++ < CNT100MS ) ) {
			UcSndDat = F40_RdStatus(ctrl, 1);
		}
	}

	return( ans );															// CheckSum OK

}

//********************************************************************************
// Function Name 	: F40_RdStatus
//********************************************************************************
uint8_t	F40_RdStatus( struct msm_ois_ctrl_t *ctrl, uint8_t UcStBitChk )
{
	uint32_t	UlReadVal ;

	RamRead32A( ctrl, 0xF100 , &UlReadVal );
	if( UcStBitChk ){
		UlReadVal &= READ_STATUS_INI ;
	}
	if( !UlReadVal ){
		return( SUCCESS );
	}else{
		return( FAILURE );
	}
}

//********************************************************************************
// Function Name 	: F40_WriteCalData
//********************************************************************************
uint8_t F40_WriteCalData( struct msm_ois_ctrl_t *ctrl, uint32_t * BufDat, uint32_t * ChkSum )
{
	uint16_t	UsSize = 0, UsNum;
	uint8_t ans = 0;
	uint32_t	UlReadVal = 0;

	*ChkSum = 0;

	ans = F40_UnlockCodeSet(ctrl);
	if ( ans != 0 ) return (ans);

	F40_IOWrite32A( ctrl, FLASHROM_F40_WDATH, 0x000000FF ) ;

	do{
		F40_IOWrite32A( ctrl, FLASHROM_F40_ACSCNT, (FLASH_ACCESS_SIZE - 1) ) ;
		F40_IOWrite32A( ctrl, FLASHROM_F40_ADR, 0x00010040 + UsSize ) ;
		F40_IOWrite32A( ctrl, FLASHROM_F40_CMD, 2) ;
		for( UsNum = 0; UsNum < FLASH_ACCESS_SIZE; UsNum++ )
		{
			F40_IOWrite32A( ctrl, FLASHROM_F40_WDATL,  BufDat[ UsSize ] ) ;
			do {
				F40_IORead32A( ctrl, FLASHROM_F40_INT, &UlReadVal );
			}while ( (UlReadVal & 0x00000020) != 0 );

			*ChkSum += BufDat[ UsSize++ ];
		}
	}while (UsSize < 64);

	ans = F40_UnlockCodeClear(ctrl);

	return( ans );
}