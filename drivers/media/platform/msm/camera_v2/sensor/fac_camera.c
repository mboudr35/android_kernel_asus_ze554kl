#include "fac_camera.h"

#ifdef FAC_DEBUG
#undef CDBG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#undef CDBG
#define CDBG(fmt, args...) pr_debug(fmt, ##args)
#endif

u8 front_otp_data[OTP_DATA_LEN_BYTE];	
u8 rear_otp_data[OTP_DATA_LEN_BYTE];	
u8 rear_otp_1_data[OTP_DATA_LEN_BYTE];

u8 front_otp_data_bank1[OTP_DATA_LEN_BYTE];	//store front_otp bank1 data
u8 rear_otp_data_bank1[OTP_DATA_LEN_BYTE];	//store rear_otp bank1 data
u8 rear_otp_1_data_bank1[OTP_DATA_LEN_BYTE];

u8 front_otp_data_bank2[OTP_DATA_LEN_BYTE];	//store front_otp bank2 data
u8 rear_otp_data_bank2[OTP_DATA_LEN_BYTE];	//store rear_otp bank2 data
u8 rear_otp_1_data_bank2[OTP_DATA_LEN_BYTE];

u8 front_otp_data_bank3[OTP_DATA_LEN_BYTE];	//store front_otp bank3 data
u8 rear_otp_data_bank3[OTP_DATA_LEN_BYTE];	//store rear_otp bank3 data
u8 rear_otp_1_data_bank3[OTP_DATA_LEN_BYTE];

u8 rear_otp_vcm_version; //ASUS_BSP Lucien +++: Using service to auto update ois fw


#define PDAF_INFO_MAX_SIZE 1024
uint16_t g_sensor_id[MAX_CAMERAS]={0};
struct msm_sensor_ctrl_t *g_sensor_ctrl[MAX_CAMERAS]={0};

static struct proc_dir_entry *status_proc_file=NULL;
struct proc_dir_entry *pdaf_file;
char *pdaf_info = NULL;

//ASUS_BSP +++ Bryant "Implement frontCam OTP"
#define SENSOR_MAX_RETRIES      50
#define FRONT_MODULE_OTP_SIZE 482
#define MODULE_OTP_SIZE 96
#define BANK_SIZE 32
static  char front_module_otp[FRONT_MODULE_OTP_SIZE];
static  char back2_module_otp[FRONT_MODULE_OTP_SIZE];
//ASUS_BSP --- Bryant "Implement frontCam OTP"

//dump eeprom data to OTP info
extern struct msm_eeprom_memory_block_t g_rear_eeprom_mapdata;
extern struct msm_eeprom_memory_block_t g_front_eeprom_mapdata;

// Sheldon_Li add for init ctrl++
int fcamera_power_up( struct msm_sensor_ctrl_t *s_ctrl)
{  
	  int rc = 0;
	  
	  if (!s_ctrl) {
		pr_err("%s:%d failed: %p\n",
			__func__, __LINE__, s_ctrl);
		return -EINVAL;
	  }
	  
	  rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
	  if(rc == 0){
		s_ctrl->sensor_state = MSM_SENSOR_POWER_UP;
	  }
	  return rc;
}

int fcamera_power_down( struct msm_sensor_ctrl_t *s_ctrl)
{  
	  int rc = 0;

	   if (!s_ctrl) {
		pr_err("%s:%d failed: %p\n",
			__func__, __LINE__, s_ctrl);
		return -EINVAL;
	  }
	   
	  rc = s_ctrl->func_tbl->sensor_power_down(s_ctrl);			      
	  if(rc == 0){
		s_ctrl->sensor_state = MSM_SENSOR_POWER_DOWN;
	  }
	  return rc;
}
// Sheldon_Li add for init ctrl--

void set_sensor_info(int camera_id,struct msm_sensor_ctrl_t* ctrl,uint16_t sensor_id)
{
	if(camera_id<MAX_CAMERAS)
	{
		g_sensor_id[camera_id]=sensor_id;
		g_sensor_ctrl[camera_id]=ctrl;
	}
}

int msm_sensor_testi2c(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	int power_self = 0;
	struct msm_camera_power_ctrl_t *power_info;
	struct msm_camera_i2c_client *sensor_i2c_client;
	struct msm_camera_slave_info *slave_info;
	const char *sensor_name;
	uint16_t chipid;
	uint32_t retry = 0;

	if (!s_ctrl) {
		pr_err("%s:%d failed: %p\n",
			__func__, __LINE__, s_ctrl);
		return -EINVAL;
	}

	power_info = &s_ctrl->sensordata->power_info;
	sensor_i2c_client = s_ctrl->sensor_i2c_client;
	slave_info = s_ctrl->sensordata->slave_info;
	sensor_name = s_ctrl->sensordata->sensor_name;

	if (!power_info || !sensor_i2c_client || !slave_info ||
		!sensor_name) {
		pr_err("%s:%d failed: %p %p %p %p\n",
			__func__, __LINE__, power_info,
			sensor_i2c_client, slave_info, sensor_name);
		return -EINVAL;
	}

	     if(s_ctrl->sensor_state == MSM_SENSOR_POWER_DOWN){
		   rc = fcamera_power_up(s_ctrl); //power on first
		   if(rc < 0){
	  		pr_err("%s:%d camera power up failed!\n",__func__, __LINE__);
			return -EINVAL;
		   }
		   power_self = 1;
	     }
	   
	for (retry = 0; retry < 5; retry++) {
	  rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
		sensor_i2c_client, 0x0100,
		&chipid, MSM_CAMERA_I2C_WORD_DATA);
		if (rc < 0) {
			   rc = fcamera_power_down(s_ctrl); //power down if read failed
			   if(rc < 0){
		  		pr_err("%s:%d camera power down failed!\n",__func__, __LINE__);
				return rc;
			   }
			return -EINVAL;
		}

		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x0100,
		chipid, MSM_CAMERA_I2C_WORD_DATA);
		if (rc < 0) {		
			   rc = fcamera_power_down(s_ctrl); //power down if write failed
			   if(rc < 0){
		  		pr_err("%s:%d camera power down failed!\n",__func__, __LINE__);
				return rc;
			   }
			return -EINVAL;
		}
	}
	   if(s_ctrl->sensor_state == MSM_SENSOR_POWER_UP && power_self == 1){
		   rc = fcamera_power_down(s_ctrl); //power down end
		   if(rc < 0){
	  		pr_err("%s:%d camera power down failed!\n",__func__, __LINE__);
			return rc;
		   }
		   power_self = 0;
	   }
	return rc;
}

int read_otp_asus(int sensor_id, int cameraID)
{	
	int rc;
	printk("%s : E , sensor_id = %d\n",__func__, sensor_id);
         if(sensor_id == SENSOR_ID_IMX362){
		rc = imx362_otp_read(cameraID);
		if(rc < 0){
  			pr_err("%s:%d imx362 otp read failed!\n",__func__, __LINE__);
		}
	}
	else if(sensor_id == SENSOR_ID_S5K3M3){
		rc = s5k3m3_otp_read(cameraID);
		if(rc < 0){
  			pr_err("%s:%d s5k3m3_otp_read read failed!\n",__func__, __LINE__);
		}
	}
	else if(sensor_id == SENSOR_ID_IMX214){
		rc = imx214_otp_read(cameraID);
		if(rc < 0){
  			pr_err("%s:%d imx214_otp_read read failed!\n",__func__, __LINE__);
		}
	}else if(sensor_id == SENSOR_ID_OV8856){
		rc = ov8856_otp_read(cameraID);
		if(rc < 0){
  			pr_err("%s:%d ov8856_otp_read failed!\n",__func__, __LINE__);
		}
	}
	printk("%s : X\n",__func__);
	return 0;	
}

int t4k37_35_otp_read(void)
{
	struct msm_sensor_ctrl_t *s_ctrl = g_sensor_ctrl[CAMERA_0];
	struct msm_camera_i2c_client *sensor_i2c_client;
	const char *sensor_name;
	int rc = 0,i,j;
	//int flag = 3; //find the otp data index
	u16 reg_val = 0;
	u16 mFind=0;
	u16 mFlag=0;
	u8 read_value[OTP_DATA_LEN_BYTE];
	sensor_i2c_client = s_ctrl->sensor_i2c_client;
	sensor_name = s_ctrl->sensordata->sensor_name;
	
		//1.open stream
		rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
			sensor_i2c_client, 0x0100,
			&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x0100,
			reg_val|0x0001, MSM_CAMERA_I2C_BYTE_DATA);
		msleep(5);
	
		//2.get access to otp, and set read mode 1xxxx101
		
	
		//2.1 set clk
		rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
			sensor_i2c_client, 0x3545,
			&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3545,
			(reg_val|0x0c)&0xec, MSM_CAMERA_I2C_BYTE_DATA);
	
		//3.read otp ,it will spend one page(64 Byte) to store 24 * 8 bite
		for ( j =2 ; j >=0; j--){
			
			memset(read_value,0,sizeof(read_value));
				//3.1 set page num
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
					sensor_i2c_client, 0x3502,
					0x0000 + j, MSM_CAMERA_I2C_BYTE_DATA);
			if (rc < 0) pr_err("select page failed\n");
			msleep(5);
			//3.2 get access to otp, and set read mode 1xxxx101
			rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
			sensor_i2c_client, 0x3500,
			&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x3500,
			(reg_val|0x85)&0xfd, MSM_CAMERA_I2C_BYTE_DATA);
			msleep(5);
			for ( i = 0;i < OTP_DATA_LEN_WORD ; i++){
				reg_val = 0;
	
				//3.3 read page otp data
				rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
					sensor_i2c_client, 0x3504 + i * 2,
					&reg_val, MSM_CAMERA_I2C_WORD_DATA);
				read_value[i*2] = reg_val>>8;
				read_value[i*2 +1] = reg_val&0x00ff;
				
				if(rc<0)
				pr_err( "failed to read OTP data j:%d,i:%d\n",j,i);
				if(reg_val != 0)
				{
					if(mFind==0)
					{	
						mFind = 1;
						mFlag=j;
					}	
				}
			}
		
			if(j==0)
			memcpy(rear_otp_data_bank1, read_value, OTP_DATA_LEN_BYTE);
			else if(j==1)
			memcpy(rear_otp_data_bank2, read_value, OTP_DATA_LEN_BYTE);
			else 
			memcpy(rear_otp_data_bank3, read_value, OTP_DATA_LEN_BYTE);
		
		}
		if(mFlag==2)
			memcpy(rear_otp_data,rear_otp_data_bank3, OTP_DATA_LEN_BYTE);
		else if(mFlag==1)
			memcpy(rear_otp_data,rear_otp_data_bank2, OTP_DATA_LEN_BYTE);
		else
			memcpy(rear_otp_data,rear_otp_data_bank1, OTP_DATA_LEN_BYTE);
	
		//4.close otp access
		rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
				sensor_i2c_client, 0x3500,
				&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x3500,
				reg_val&0x007f, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0)pr_err("%s: %s: close otp access failed\n", __func__, sensor_name);
	
		//5.close stream
		rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
			sensor_i2c_client, 0x0100,
			&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x0100,
			reg_val|0x0000, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0)pr_err("%s: %s: close stream failed\n", __func__, sensor_name);
	
	 return rc;	
}

//ASUS_BSP Sheldon  add thermal node +++ 
uint16_t sensor_read_temperature(uint16_t sensor_id, int cameraID)
{
	int rc = -1;
 	static uint16_t reg_data = -1;
	uint16_t reg_val = -1;  

         struct msm_sensor_ctrl_t *s_ctrl = g_sensor_ctrl[cameraID];
         struct msm_camera_i2c_client *sensor_i2c_client;

	struct timeval tv_now;
	static struct timeval tv_prv;
	unsigned long long val;

	if(s_ctrl != NULL){
		mutex_lock(s_ctrl->msm_sensor_mutex);
		if(s_ctrl->sensor_state == MSM_SENSOR_POWER_UP){
			sensor_i2c_client = s_ctrl->sensor_i2c_client;
			if(sensor_i2c_client != NULL){
				do_gettimeofday(&tv_now);
				val = (tv_now.tv_sec*1000000000LL+tv_now.tv_usec*1000)-(tv_prv.tv_sec*1000000000LL+tv_prv.tv_usec*1000);

				//[sheldon] : read circle 50ms	
				if(val >= 50){ 
					
					//[1] : Temperature control enable
					 rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
				          sensor_i2c_client, 0x0138, &reg_val, MSM_CAMERA_I2C_BYTE_DATA);
					if(rc<0) {
						pr_err("[camera_thermal]read enable failed!\n");
						mutex_unlock(s_ctrl->msm_sensor_mutex);
						return rc;
					}  
					
					rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
					 sensor_i2c_client, 0x0138, (reg_val | 0x01), MSM_CAMERA_I2C_BYTE_DATA);
					if(rc<0) {
						 pr_err("[camera_thermal] set enable failed!\n");
						 mutex_unlock(s_ctrl->msm_sensor_mutex);
						 return rc;
					}
					
					//[2] : Temperature data output
					rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
						sensor_i2c_client, 0x013a, &reg_val, MSM_CAMERA_I2C_BYTE_DATA);
					reg_data =  (int)(reg_val&0xff);
					//pr_err("[camera_thermal] temperature = 0x%02x\n",reg_data);
					
					if (rc < 0) {
						reg_data = 0;
						pr_err("[camera_thermal] %s: read reg failed\n", __func__);
						mutex_unlock(s_ctrl->msm_sensor_mutex);
						return rc;
					}else{
						tv_prv.tv_sec = tv_now.tv_sec;
						tv_prv.tv_usec = tv_now.tv_usec;
					}
					
					//[3] : Temperature control disable
					 rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
				          sensor_i2c_client, 0x0138, &reg_val, MSM_CAMERA_I2C_BYTE_DATA);
					 if(rc<0) {
						pr_err("[camera_thermal]read disable failed!\n");
						mutex_unlock(s_ctrl->msm_sensor_mutex);
						return rc;
					 } 
					
					 rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
					 sensor_i2c_client, 0x0138, (reg_val | 0x00), MSM_CAMERA_I2C_BYTE_DATA);
					 if(rc<0) {
						 pr_err("[camera_thermal] set disable failed!\n");
						 mutex_unlock(s_ctrl->msm_sensor_mutex);
						 return rc;
					 } 	
				}else{
					pr_debug( "[sz_cam]camera_thermal : cached (%d) val=%lld\n", reg_data, val);
				}
			}
		}else{
			pr_debug("camera_thermal-%d Not power up!\n",cameraID);
			reg_data = 0;
		}
		mutex_unlock(s_ctrl->msm_sensor_mutex);
	}else{
		pr_err("camera_thermal %d is NULL!\n",cameraID);
		reg_data = 0;
	}
	return reg_data;
}
//ASUS_BSP Sheldon  add thermal node ---


// Sheldon_Li add for imx214\362 ++
int imx214_otp_read(int camNum)
{
	   const char *sensor_name;
	   int rc = 0,i,j,flag,b1,b2,b3;
	   int check_count=0;	
	   u16 check_status=0;
	   u16 reg_val = 0;
	   u8 read_value[OTP_DATA_LEN_BYTE];

	   

	   struct msm_sensor_ctrl_t *s_ctrl = g_sensor_ctrl[camNum];
	   struct msm_camera_i2c_client *sensor_i2c_client;

 	   printk("%s : E\n",__func__);
	   
	   if(!s_ctrl){
  		pr_err("%s:%d failed: %p\n",__func__, __LINE__, s_ctrl);
		return rc;
	   }

	   sensor_i2c_client = s_ctrl->sensor_i2c_client;
	   sensor_name = s_ctrl->sensordata->sensor_name;

	   rc = fcamera_power_up(s_ctrl); //power on first
	   if(rc < 0){
  		pr_err("%s:%d camera power up failed!\n",__func__, __LINE__);
		return -EINVAL;
	   }
	 
	   b1=0;b2=0;b3=0;
	   for(j = 0;  j < 15;  j++)
	   {
		        flag = 0; //clear flag
			//1.Set the NVM page number,N(=0-15)
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x0a02,
				(j), MSM_CAMERA_I2C_BYTE_DATA);
			if(rc < 0) pr_err("%s : Set the NVM page number failed!\n",__func__);
	
			
		        //2.Turn ON OTP Read MODE
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x0a00,
				0x01, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("%s :  Turn ON OTP Read MODE failed!\n",__func__);
			
			do {
				rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
						sensor_i2c_client, 0x0A01,
						&check_status, MSM_CAMERA_I2C_BYTE_DATA);
						
				usleep_range(300, 500);
				check_count++;
			} while (((check_status & 0x1d) != 0) && (check_count <= 10));
		
			//3.Read Data0-Data63 in the page N, as required.
			if((check_status & 0x1d) != 0){
				memset(read_value,0,sizeof(read_value));
				for ( i = 0; i < OTP_DATA_LEN_WORD ;  i++){
						reg_val = 0;
						// read page otp data
						rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
							sensor_i2c_client, 0x0a04 + i * 2,
							&reg_val, MSM_CAMERA_I2C_WORD_DATA);
						read_value[i*2] = reg_val>>8;
						read_value[i*2 +1] = reg_val&0x00ff;
						
						if(rc<0)	pr_err( "[%s]failed to read OTP data j:%d,i:%d\n",sensor_name,j,i);
						
						if(reg_val != 0)
						{	 pr_debug( "[sz_cam][%s] read OTP data j:%d,i:%d = %d\n",sensor_name,j,i,reg_val);
							 flag = 1;
						}
				 }
				
				if((flag == 1) && (b1 == 0))
				{	
					//sheldon_debug( "[sheldon]Get OTP data in page : %d\n",j);
					if(camNum == 0){
						memcpy(rear_otp_data_bank1, read_value, OTP_DATA_LEN_BYTE);		
					}
					else if(camNum == 1){
						memcpy(front_otp_data_bank1, read_value, OTP_DATA_LEN_BYTE);
					}
					else if(camNum == 2){
						memcpy(rear_otp_1_data_bank1, read_value, OTP_DATA_LEN_BYTE);		
					}
					b1 = 1;
				}
				else if((flag == 1) && (b2 == 0))
				{ 	
					//sheldon_debug( "[sheldon]Get OTP data in page : %d\n",j);
					if(camNum == 0){
						memcpy(rear_otp_data_bank2, read_value, OTP_DATA_LEN_BYTE);		
					}
					else if(camNum == 1){
						memcpy(front_otp_data_bank2, read_value, OTP_DATA_LEN_BYTE);			
					}
					else if(camNum == 2){
						memcpy(rear_otp_1_data_bank2, read_value, OTP_DATA_LEN_BYTE);
					}
					b2 = 1;
				}
				else if((flag == 1) && (b3 == 0))
				{ 	
					//sheldon_debug( "[sheldon]Get OTP data in page : %d\n",j);
					if(camNum == 0){
						memcpy(rear_otp_data_bank3, read_value, OTP_DATA_LEN_BYTE);		
					}
					else if(camNum == 1){
						memcpy(front_otp_data_bank3, read_value, OTP_DATA_LEN_BYTE);			
					}
					else if(camNum == 2){
						memcpy(rear_otp_1_data_bank3, read_value, OTP_DATA_LEN_BYTE);
					}		
					b3 = 1;
				}									
			}
	   }

	  if(b3 == 1){
	   	if(camNum == 0)
			memcpy(rear_otp_data,rear_otp_data_bank3, OTP_DATA_LEN_BYTE); 
		 else if(camNum == 1)
		 	memcpy(front_otp_data,front_otp_data_bank3, OTP_DATA_LEN_BYTE); 
		 else if(camNum == 2)
		 	memcpy(rear_otp_1_data,rear_otp_1_data_bank3, OTP_DATA_LEN_BYTE); 
	   } else if(b2 == 1){
	  	if(camNum == 0)
			memcpy(rear_otp_data,rear_otp_data_bank2, OTP_DATA_LEN_BYTE); 
		else if(camNum == 1)
		 	memcpy(front_otp_data,front_otp_data_bank2, OTP_DATA_LEN_BYTE); 
		 else if(camNum == 2) 	
		         memcpy(rear_otp_1_data,rear_otp_1_data_bank2, OTP_DATA_LEN_BYTE); 
	  } else if(b1 == 1){
	  	if(camNum == 0)
			memcpy(rear_otp_data,rear_otp_data_bank1, OTP_DATA_LEN_BYTE);
		else if(camNum == 1)
		 	memcpy(front_otp_data,front_otp_data_bank1, OTP_DATA_LEN_BYTE); 
		 else if(camNum == 2)
		 	memcpy(rear_otp_1_data,rear_otp_1_data_bank1, OTP_DATA_LEN_BYTE); 
	  }

	  if(s_ctrl &&  s_ctrl->sensor_state==MSM_SENSOR_POWER_UP)
	  {
		if( fcamera_power_down(s_ctrl)<0)
		{
			pr_err("%s camera power down failed!\n",__func__);
		}
	  }
	   
	   printk("%s : X\n",__func__);
	   return rc;
}

int imx362_otp_read(uint16_t camNum)
{

#ifdef HADES_OTP_MODE	  
	   pr_info("%s:%d :  HADES_OTP_MODE\n",__func__, __LINE__);
	   return 0;
#else

	   const char *sensor_name;
	   int rc = 0,i,j,flag,b1,b2,b3;
	   int check_count=0;	 
	   u16 check_status=0;
	   u16 reg_val = 0;
	   u8 read_value[OTP_DATA_LEN_BYTE];

	   struct msm_sensor_ctrl_t *s_ctrl = g_sensor_ctrl[camNum];
	   struct msm_camera_i2c_client *sensor_i2c_client;

	   if(!s_ctrl){
  		pr_err("%s:%d failed: %p\n",__func__, __LINE__, s_ctrl);
		return rc;
	   }

	   sensor_i2c_client = s_ctrl->sensor_i2c_client;
	   sensor_name = s_ctrl->sensordata->sensor_name;

	   rc = fcamera_power_up(s_ctrl); //power on first
	   if(rc < 0){
  		pr_err("%s:%d camera power up failed!\n",__func__, __LINE__);
		return -EINVAL;
	   }
	 
	   b1=0;b2=0;b3=0;
	   for(j = 0;  j < 54;  j++)
	   {
		        flag = 0; //clear flag
			//1.Set the NVM page number,N(=0-54)
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x0a02,
				(j), MSM_CAMERA_I2C_BYTE_DATA);
			if(rc < 0) pr_err("%s : Set the NVM page number failed!\n",__func__);
			
		        //2.Turn ON OTP Read MODE
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x0a00,
				0x01, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("%s :  Set up for NVM read failed!\n",__func__);

			do {
				rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
						sensor_i2c_client, 0x0A01,
						&check_status, MSM_CAMERA_I2C_BYTE_DATA);
						
				usleep_range(300, 500);
				check_count++;
			} while (((check_status & 0x1d) != 0) && (check_count <= 10));

			//3.Read Data0-Data63 in the page N, as required.
			if((check_status & 0x1d) != 0){
				memset(read_value,0,sizeof(read_value));
				for ( i = 0; i < OTP_DATA_LEN_WORD ;  i++){
						reg_val = 0;
						// read page otp data
						rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
							sensor_i2c_client, 0x0a04 + i * 2,
							&reg_val, MSM_CAMERA_I2C_WORD_DATA);
						read_value[i*2] = reg_val>>8;
						read_value[i*2 +1] = reg_val&0x00ff;
						
						if(rc<0)	pr_err( "[%s]failed to read OTP data j:%d,i:%d\n",sensor_name,j,i);
						
						if(reg_val != 0)
						{
							 flag = 1;					
						}
			  	}
				
				if((flag == 1) && (b1 == 0))
				{	
					//sheldon_debug( "[sheldon]Get OTP data in page : %d\n",j);
					if(camNum == 0){
						memcpy(rear_otp_data_bank1, read_value, OTP_DATA_LEN_BYTE);		
					}
					else if(camNum == 1){
						memcpy(front_otp_data_bank1, read_value, OTP_DATA_LEN_BYTE);
					}
					else if(camNum == 2){
						memcpy(rear_otp_1_data_bank1, read_value, OTP_DATA_LEN_BYTE);		
					}
					b1 = 1;
				}
				else if((flag == 1) && (b2 == 0))
				{ 	
					//sheldon_debug( "[sheldon]Get OTP data in page : %d\n",j);
					if(camNum == 0){
						memcpy(rear_otp_data_bank2, read_value, OTP_DATA_LEN_BYTE);		
					}
					else if(camNum == 1){
						memcpy(front_otp_data_bank2, read_value, OTP_DATA_LEN_BYTE);			
					}
					else if(camNum == 2){
						memcpy(rear_otp_1_data_bank2, read_value, OTP_DATA_LEN_BYTE);
					}
					b2 = 1;
				}
				else if((flag == 1) && (b3 == 0))
				{ 	
					//sheldon_debug( "[sheldon]Get OTP data in page : %d\n",j);
					if(camNum == 0){
						memcpy(rear_otp_data_bank3, read_value, OTP_DATA_LEN_BYTE);		
					}
					else if(camNum == 1){
						memcpy(front_otp_data_bank3, read_value, OTP_DATA_LEN_BYTE);			
					}
					else if(camNum == 2){
						memcpy(rear_otp_1_data_bank3, read_value, OTP_DATA_LEN_BYTE);
					}		
					b3 = 1;
				}									
			}
	   }

	  if(b3 == 1){
	   	if(camNum == 0)
			memcpy(rear_otp_data,rear_otp_data_bank3, OTP_DATA_LEN_BYTE); 
		 else if(camNum == 1)
		 	memcpy(front_otp_data,front_otp_data_bank3, OTP_DATA_LEN_BYTE); 
		 else if(camNum == 2)
		 	memcpy(rear_otp_1_data,rear_otp_1_data_bank3, OTP_DATA_LEN_BYTE); 
	   } else if(b2 == 1){
	  	if(camNum == 0)
			memcpy(rear_otp_data,rear_otp_data_bank2, OTP_DATA_LEN_BYTE); 
		else if(camNum == 1)
		 	memcpy(front_otp_data,front_otp_data_bank2, OTP_DATA_LEN_BYTE); 
		 else if(camNum == 2) 	
		         memcpy(rear_otp_1_data,rear_otp_1_data_bank2, OTP_DATA_LEN_BYTE); 
	  } else if(b1 == 1){
	  	if(camNum == 0)
			memcpy(rear_otp_data,rear_otp_data_bank1, OTP_DATA_LEN_BYTE);
		else if(camNum == 1)
		 	memcpy(front_otp_data,front_otp_data_bank1, OTP_DATA_LEN_BYTE); 
		 else if(camNum == 2)
		 	memcpy(rear_otp_1_data,rear_otp_1_data_bank1, OTP_DATA_LEN_BYTE); 
	  }
		
	if(s_ctrl &&  s_ctrl->sensor_state==MSM_SENSOR_POWER_UP)
	{
		if( fcamera_power_down(s_ctrl)<0)
		{
			pr_err("%s camera power down failed!\n",__func__);
		}
	} 
	return rc;
#endif		
}

// Sheldon_Li add for imx214\362 --

// Sheldon_Li add for s5k3m3_otp_read++ 
int s5k3m3_otp_read(int camNum)
{

#ifdef HADES_OTP_MODE	  
	   pr_info("%s:%d :    HADES_OTP_MODE\n",__func__, __LINE__);
	   return 0;
#else

	   const char *sensor_name;
	   int rc = 0,i,j,flag,b1,b2,b3;
	   u16 reg_val = 0;
	   u8 read_value[OTP_DATA_LEN_BYTE];

	   struct msm_sensor_ctrl_t *s_ctrl = g_sensor_ctrl[camNum];
	   struct msm_camera_i2c_client *sensor_i2c_client;

	   printk("%s : E\n",__func__);

	   if(!s_ctrl){
  		pr_err("%s:%d failed: %p\n",__func__, __LINE__, s_ctrl);
		return rc;
	   }

	   sensor_i2c_client = s_ctrl->sensor_i2c_client;
	   sensor_name = s_ctrl->sensordata->sensor_name;

	   rc = fcamera_power_up(s_ctrl); //power on first
	   if(rc < 0){
  		pr_err("%s:%d camera power up failed!\n",__func__, __LINE__);
		return -EINVAL;
	   }

	   
	 //0.open stream
	 rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
			sensor_i2c_client, 0x0100,
	  		&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
	 rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
			sensor_i2c_client, 0x0100,
			reg_val|0x01, MSM_CAMERA_I2C_BYTE_DATA);
		
	   b1=0;b2=0;b3=0;
	   for(j = 0;  j < 63;  j++)
	   {
		        flag = 0; //clear flag	
			//1.Set the NVM page number,N(=0-63)
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x0a02,
				(j<<8), MSM_CAMERA_I2C_WORD_DATA);
			if(rc < 0) pr_err("%s : Set the NVM page number failed!\n",__func__);
			
		         //2.Turn ON OTP Read MODE
			rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
				sensor_i2c_client, 0x0a00,
				&reg_val, MSM_CAMERA_I2C_WORD_DATA);
			if(rc<0)  pr_err("%s : Read for NVM read failed!\n",__func__);
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x0a00,
				(reg_val|0x0100), MSM_CAMERA_I2C_WORD_DATA);
			if(rc<0) pr_err("%s :  Set up for NVM read failed!\n",__func__);
	
			//3.Read Data0-Data63 in the page N, as required.
			memset(read_value,0,sizeof(read_value));
			for ( i = 0; i < OTP_DATA_LEN_WORD ;  i++){
					reg_val = 0;
					// read page otp data
					rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
						sensor_i2c_client, 0x0a04 + i * 2,
						&reg_val, MSM_CAMERA_I2C_WORD_DATA);
					read_value[i*2] = reg_val>>8;
					read_value[i*2 +1] = reg_val&0x00ff;
					
					if(rc<0)	pr_err( "[%s]failed to read OTP data j:%d,i:%d\n",sensor_name,j,i);
					
					if(reg_val != 0)
					{
						 flag = 1;
					}
			  }

			 //4.close stream
			 rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
				sensor_i2c_client, 0x0100,
	  			&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
			 rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x0100,
				reg_val|0x00, MSM_CAMERA_I2C_BYTE_DATA);

			//5.make initial state
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x0a00,
				0x0000, MSM_CAMERA_I2C_WORD_DATA);
			
			if((flag == 1) && (b1 == 0))
				{	
					//sheldon_debug( "[sheldon]Get OTP data in page : %d\n",j);
					if(camNum == 0){
						memcpy(rear_otp_data_bank1, read_value, OTP_DATA_LEN_BYTE);		
					}
					else if(camNum == 1){
						memcpy(front_otp_data_bank1, read_value, OTP_DATA_LEN_BYTE);			
					}
					else if(camNum == 2){					
						memcpy(rear_otp_1_data_bank1, read_value, OTP_DATA_LEN_BYTE);
					}
					b1 = 1;
				}
				else if((flag == 1) && (b2 == 0))
				{ 	
					//sheldon_debug( "[sheldon]Get OTP data in page : %d\n",j);
					if(camNum == 0){
						memcpy(rear_otp_data_bank2, read_value, OTP_DATA_LEN_BYTE);		
					}
					else if(camNum == 1){
						memcpy(front_otp_data_bank2, read_value, OTP_DATA_LEN_BYTE);		
					}
					else if(camNum == 2){	
						memcpy(rear_otp_1_data_bank2, read_value, OTP_DATA_LEN_BYTE);
					}
					b2 = 1;
				}
				else if((flag == 1) && (b3 == 0))
				{ 	
					//sheldon_debug( "[sheldon]Get OTP data in page : %d\n",j);
					if(camNum == 0){
						memcpy(rear_otp_data_bank3, read_value, OTP_DATA_LEN_BYTE);		
					}
					else if(camNum == 1){
						memcpy(front_otp_data_bank3, read_value, OTP_DATA_LEN_BYTE);		
					}
					else if(camNum == 2){
						memcpy(rear_otp_1_data_bank3, read_value, OTP_DATA_LEN_BYTE);
					}		
					b3 = 1;
				}		
	   }

	  if(b3 == 1){
	   	if(camNum == 0)
			memcpy(rear_otp_data,rear_otp_data_bank3, OTP_DATA_LEN_BYTE); 
		 else if(camNum == 1)
		 	memcpy(front_otp_data,front_otp_data_bank3, OTP_DATA_LEN_BYTE); 
		 else if(camNum == 2) 	
			 memcpy(rear_otp_1_data,rear_otp_1_data_bank3, OTP_DATA_LEN_BYTE); 
	   } else if(b2 == 1){
	  	if(camNum == 0)
			memcpy(rear_otp_data,rear_otp_data_bank2, OTP_DATA_LEN_BYTE); 
		else if(camNum == 1)
		 	memcpy(front_otp_data,front_otp_data_bank2, OTP_DATA_LEN_BYTE); 
		 else if(camNum == 2)	
		 	memcpy(rear_otp_1_data,rear_otp_1_data_bank2, OTP_DATA_LEN_BYTE); 
	  } else if(b1 == 1){
	  	if(camNum == 0)
			memcpy(rear_otp_data,rear_otp_data_bank1, OTP_DATA_LEN_BYTE);
		else if(camNum == 1)
		 	memcpy(front_otp_data,front_otp_data_bank1, OTP_DATA_LEN_BYTE); 
		 else if(camNum == 2) 	
			 memcpy(rear_otp_1_data,rear_otp_1_data_bank1, OTP_DATA_LEN_BYTE); 
	  }
	  	
	   if(s_ctrl &&  s_ctrl->sensor_state==MSM_SENSOR_POWER_UP)
	   {
		if( fcamera_power_down(s_ctrl)<0)
		{
			pr_err("%s camera power down failed!\n",__func__);
		}
	   }   
	   printk("%s : X\n",__func__);  
	   return rc;
#endif
}
// Sheldon_Li add for s5k3m3_otp_read --

//ASUS_BSP +++ Bryant "Implement frontCam OTP"
int sensor_read_reg(struct msm_sensor_ctrl_t  *s_ctrl, u16 addr, u16 *val)
{
	int err;
	err = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(s_ctrl->sensor_i2c_client, addr, val, MSM_CAMERA_I2C_BYTE_DATA);
	//pr_err("Bryant %s: sensor_read_reg 0x%x\n", __func__, *val);
	if(err <0)
		return -EINVAL;

	else return 0;
}

int sensor_write_reg(struct msm_sensor_ctrl_t  *s_ctrl, u16 addr, u16 val)
{
	int err;
	int retry = 0;
	do {
		err =s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(s_ctrl->sensor_i2c_client, addr, val, MSM_CAMERA_I2C_BYTE_DATA);

		if (err == 0){return 0;}
		retry++;
		pr_err("i2c transfer failed, retrying %x %x\n", addr, val);
		msleep(1);
	} while (retry <= SENSOR_MAX_RETRIES);

	if(err == 0) {
		pr_err("%s(%d): i2c_transfer error, but return 0!?\n", __FUNCTION__, __LINE__);
		err = 0xAAAA;
	}

	return err;
}


static struct msm_camera_i2c_reg_array otp_reg_array[] = {
		{0x3D84, 0xC0},
		{0x3D88, 0x70},
		{0x3D89, 0x00},
		{0x3D8A, 0x70},
		{0x3D8B, 0x6F},
		{0x3D85, 0x06},
		{0x0100, 0x01},
		{0x3D81, 0x01},
};

static  struct msm_camera_i2c_reg_setting otp_settings = {
  .reg_setting = otp_reg_array,
  .size = ARRAY_SIZE(otp_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 20,
};

int ov8856_otp_read(int camNum)
{
 	int   i, j, package_num;
 	int32_t rc = 0;
	u16 bank_value[MODULE_OTP_SIZE], temp;
	struct msm_sensor_ctrl_t  *s_ctrl = g_sensor_ctrl[camNum];

	package_num = 3;
	//set 0x0103 to 1, make sure reset sensor as default
	rc = sensor_write_reg(s_ctrl, 0x0103, 0x01);
	if (rc) {
		pr_err("%s: set 0x0103 to 0x01\n", __func__);
		return rc;
	}

	//set 0x5001[3] to 0
	sensor_read_reg(s_ctrl, 0x5001, &temp);
	rc = sensor_write_reg(s_ctrl, 0x5001, (0x00&0x08)|(temp&(~0x08)));
	if (rc) {
		pr_err("%s: set 0x5001[3] to 0\n", __func__);
		return rc;
	}

	//read OTP into buffer
	rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(s_ctrl->sensor_i2c_client, &otp_settings);
	 if (rc < 0) {
		 pr_err("%s : read OTP into buffer failed\n", __func__);
	 }

	for(j = 0; j < package_num; j++) {
		for (i = 0; i < BANK_SIZE; i++) {
			sensor_read_reg(s_ctrl, 0x7010+i+j*BANK_SIZE, &bank_value[i+j*BANK_SIZE]);
			//pr_err("Bryant %s: bank_value[%d]=%x", __func__, i, bank_value[i+j*BANK_SIZE]);
		}
	}

	if (camNum == 1){
		snprintf(front_module_otp, sizeof(front_module_otp)
			, "0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n"
			, bank_value[0]&0xFF, bank_value[1]&0xFF, bank_value[2]&0xFF, bank_value[3]&0xFF, bank_value[4]&0xFF
			, bank_value[5]&0xFF, bank_value[6]&0xFF, bank_value[7]&0xFF, bank_value[8]&0xFF, bank_value[9]&0xFF
			, bank_value[10]&0xFF, bank_value[11]&0xFF, bank_value[12]&0xFF, bank_value[13]&0xFF, bank_value[14]&0xFF
			, bank_value[15]&0xFF, bank_value[16]&0xFF, bank_value[17]&0xFF, bank_value[18]&0xFF, bank_value[19]&0xFF
			, bank_value[20]&0xFF, bank_value[21]&0xFF, bank_value[22]&0xFF, bank_value[23]&0xFF
			, bank_value[24]&0xFF, bank_value[25]&0xFF, bank_value[26]&0xFF, bank_value[27]&0xFF, bank_value[28]&0xFF
			, bank_value[29]&0xFF, bank_value[30]&0xFF, bank_value[31]&0xFF, bank_value[32]&0xFF, bank_value[33]&0xFF
			, bank_value[34]&0xFF, bank_value[35]&0xFF, bank_value[36]&0xFF, bank_value[37]&0xFF, bank_value[38]&0xFF
			, bank_value[39]&0xFF, bank_value[40]&0xFF, bank_value[41]&0xFF, bank_value[42]&0xFF, bank_value[43]&0xFF
			, bank_value[44]&0xFF, bank_value[45]&0xFF, bank_value[46]&0xFF, bank_value[47]&0xFF
			, bank_value[48]&0xFF, bank_value[49]&0xFF, bank_value[50]&0xFF, bank_value[51]&0xFF, bank_value[52]&0xFF
			, bank_value[53]&0xFF, bank_value[54]&0xFF, bank_value[55]&0xFF, bank_value[56]&0xFF, bank_value[57]&0xFF
			, bank_value[58]&0xFF, bank_value[59]&0xFF, bank_value[60]&0xFF, bank_value[61]&0xFF, bank_value[62]&0xFF
			, bank_value[63]&0xFF, bank_value[64]&0xFF, bank_value[65]&0xFF, bank_value[66]&0xFF, bank_value[67]&0xFF
			, bank_value[68]&0xFF, bank_value[69]&0xFF, bank_value[70]&0xFF, bank_value[71]&0xFF
			, bank_value[72]&0xFF, bank_value[73]&0xFF, bank_value[74]&0xFF, bank_value[75]&0xFF, bank_value[76]&0xFF
			, bank_value[77]&0xFF, bank_value[78]&0xFF, bank_value[79]&0xFF, bank_value[80]&0xFF, bank_value[81]&0xFF
			, bank_value[82]&0xFF, bank_value[83]&0xFF, bank_value[84]&0xFF, bank_value[85]&0xFF, bank_value[86]&0xFF
			, bank_value[87]&0xFF, bank_value[88]&0xFF, bank_value[89]&0xFF, bank_value[90]&0xFF, bank_value[91]&0xFF
			, bank_value[92]&0xFF, bank_value[93]&0xFF, bank_value[94]&0xFF, bank_value[95]&0xFF);
	}
	else if (camNum == 2){
		snprintf(back2_module_otp, sizeof(back2_module_otp)
			, "0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n"
			, bank_value[0]&0xFF, bank_value[1]&0xFF, bank_value[2]&0xFF, bank_value[3]&0xFF, bank_value[4]&0xFF
			, bank_value[5]&0xFF, bank_value[6]&0xFF, bank_value[7]&0xFF, bank_value[8]&0xFF, bank_value[9]&0xFF
			, bank_value[10]&0xFF, bank_value[11]&0xFF, bank_value[12]&0xFF, bank_value[13]&0xFF, bank_value[14]&0xFF
			, bank_value[15]&0xFF, bank_value[16]&0xFF, bank_value[17]&0xFF, bank_value[18]&0xFF, bank_value[19]&0xFF
			, bank_value[20]&0xFF, bank_value[21]&0xFF, bank_value[22]&0xFF, bank_value[23]&0xFF
			, bank_value[24]&0xFF, bank_value[25]&0xFF, bank_value[26]&0xFF, bank_value[27]&0xFF, bank_value[28]&0xFF
			, bank_value[29]&0xFF, bank_value[30]&0xFF, bank_value[31]&0xFF, bank_value[32]&0xFF, bank_value[33]&0xFF
			, bank_value[34]&0xFF, bank_value[35]&0xFF, bank_value[36]&0xFF, bank_value[37]&0xFF, bank_value[38]&0xFF
			, bank_value[39]&0xFF, bank_value[40]&0xFF, bank_value[41]&0xFF, bank_value[42]&0xFF, bank_value[43]&0xFF
			, bank_value[44]&0xFF, bank_value[45]&0xFF, bank_value[46]&0xFF, bank_value[47]&0xFF
			, bank_value[48]&0xFF, bank_value[49]&0xFF, bank_value[50]&0xFF, bank_value[51]&0xFF, bank_value[52]&0xFF
			, bank_value[53]&0xFF, bank_value[54]&0xFF, bank_value[55]&0xFF, bank_value[56]&0xFF, bank_value[57]&0xFF
			, bank_value[58]&0xFF, bank_value[59]&0xFF, bank_value[60]&0xFF, bank_value[61]&0xFF, bank_value[62]&0xFF
			, bank_value[63]&0xFF, bank_value[64]&0xFF, bank_value[65]&0xFF, bank_value[66]&0xFF, bank_value[67]&0xFF
			, bank_value[68]&0xFF, bank_value[69]&0xFF, bank_value[70]&0xFF, bank_value[71]&0xFF
			, bank_value[72]&0xFF, bank_value[73]&0xFF, bank_value[74]&0xFF, bank_value[75]&0xFF, bank_value[76]&0xFF
			, bank_value[77]&0xFF, bank_value[78]&0xFF, bank_value[79]&0xFF, bank_value[80]&0xFF, bank_value[81]&0xFF
			, bank_value[82]&0xFF, bank_value[83]&0xFF, bank_value[84]&0xFF, bank_value[85]&0xFF, bank_value[86]&0xFF
			, bank_value[87]&0xFF, bank_value[88]&0xFF, bank_value[89]&0xFF, bank_value[90]&0xFF, bank_value[91]&0xFF
			, bank_value[92]&0xFF, bank_value[93]&0xFF, bank_value[94]&0xFF, bank_value[95]&0xFF);
	}

	//clear OTP buffer
	for (i = 0x7000; i <= 0x706F; i++) {
		rc = sensor_write_reg(s_ctrl, i, 0);
		if (rc) {
			pr_err("%s: failed to write 0x%x = 0x%x\n", __func__, i, 0);
			return rc;
		}
	}

	//set 0x5001[3] to 1
	sensor_read_reg(s_ctrl, 0x5001, &temp);
	rc = sensor_write_reg(s_ctrl, 0x5001, (0x08&0x08)|(temp&(~0x08)));
	if (rc) {
		pr_err("%s: set 0x5001[3] to 1\n", __func__);
		return rc;
	}

	return 0;
}
//ASUS_BSP --- Bryant "Implement frontCam OTP"

#if 0
// Sheldon_Li add for ov8856 ++
int ov8856_otp_read(void)
{
	   const char *sensor_name;
	   int rc = 0,i,j,flag,b1,b2,b3;
	   u16 reg_val = 0;
	   u16 start_addr, end_addr;
	   u8 read_value[OTP_DATA_LEN_BYTE];

	   struct msm_sensor_ctrl_t *s_ctrl = g_sensor_ctrl[CAMERA_1];
	   struct msm_camera_i2c_client *sensor_i2c_client;

	   if(!s_ctrl){
  		pr_err("%s:%d failed: %p\n",__func__, __LINE__, s_ctrl);
		return rc;
	   }

	   sensor_i2c_client = s_ctrl->sensor_i2c_client;
	   sensor_name = s_ctrl->sensordata->sensor_name;

	   rc = fcamera_power_up(s_ctrl); //power on first
	   if(rc < 0){
  		pr_err("%s:%d camera power up failed!\n",__func__, __LINE__);
		return -EINVAL;
	   }
 
            //1. Reset sensor as default
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
	        	 	sensor_i2c_client, 0x0103, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0)  pr_err("%s : Reset sensor as default failed!\n",__func__);
	   
	  //2.otp_option_en = 1
		rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
		 sensor_i2c_client, 0x5001, &reg_val, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0)  pr_err("%s : read otp option failed!\n",__func__);
		
		rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
		sensor_i2c_client, 0x5001,((0x00 & 0x08) | (reg_val & (~0x08))), MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0)  pr_err("%s : write otp option failed!\n",__func__);
	  
	   b1=0;b2=0;b3=0;
	   for(j = 3;  j > 0;  j--)
	   {
		    flag = 0; //clear flag
				
		   //3.Set the read bank number
			if (j == 1) {
				start_addr = 0x7010;
				end_addr   = 0x702f;
			} else if (j == 2) {
				start_addr = 0x7030;
				end_addr   = 0x704f;
			} else if (j == 3) {
				start_addr = 0x7050;
				end_addr   = 0x706f;
			}
			
		   //4.Set OTP MODE Bit[6] : 0=Auto mode 1=Manual mode
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x3d84,
			   	0xc0, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("%s : Write OTP_MODE_CTRL registor failed!\n",__func__);


		  //5.Set OTP_REG85 : Bit[2]=load data enable  Bit[1]= load setting enable
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x3d85,
				0x06, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("%s : Write OTP_REG85  failed!\n",__func__);

		  //6.Set OTP read address
		         //start address High
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x3d88,
				(start_addr >> 8) & 0xff, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("%s : Write start address High  failed!\n",__func__);

			//start address Low
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x3d89,
				(start_addr & 0xff) , MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("%s : Write start address Low  failed!\n",__func__);

			 //end address High
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x3d8a,
				(end_addr >> 8) & 0xff, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("%s : Write end address High  failed!\n",__func__);

			//end address Low
			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x3d8b,
				(end_addr  & 0xff), MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("%s : Write end address Low  failed!\n",__func__);


		 //7.Load OTP data enable
			rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
				sensor_i2c_client, 0x3d81,
				&reg_val, MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("%s : Read data enable REG  failed!\n",__func__);

			rc = sensor_i2c_client->i2c_func_tbl->i2c_write(
				sensor_i2c_client, 0x3d81,
				(reg_val | 0x01), MSM_CAMERA_I2C_BYTE_DATA);
			if(rc<0) pr_err("%s : Write data enable REG  failed!\n",__func__);
		
			memset(read_value,0,sizeof(read_value));
			
				for ( i = 0; i < OTP_DATA_LEN_WORD ;  i++){
					reg_val = 0;
					// read page otp data
					rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
						sensor_i2c_client, start_addr + i * 2,
						&reg_val, MSM_CAMERA_I2C_WORD_DATA);
					read_value[i*2] = reg_val>>8;
					read_value[i*2 +1] = reg_val&0x00ff;
					
					if(rc<0)	pr_err( "[%s]failed to read OTP data j:%d,i:%d\n",sensor_name,j,i);
					
					if(reg_val != 0)
					{
						 flag = 1;
					}
			  }
			
			if((flag == 1) && (b1 == 0))
			{
				memcpy(front_otp_data_bank1, read_value, OTP_DATA_LEN_BYTE);			
				b1 = 1;
			}
			else if((flag == 1) && (b2 == 0))
			{
				memcpy(front_otp_data_bank2, read_value, OTP_DATA_LEN_BYTE);	
				b2 = 1;
			}
			else if((flag == 1) && (b3 == 0))
			{
				memcpy(front_otp_data_bank3, read_value, OTP_DATA_LEN_BYTE); 		
				b3 = 1;
			}
	   }
	
		  if(b3 == 1)
			memcpy(front_otp_data,front_otp_data_bank3, OTP_DATA_LEN_BYTE); // front_otp_data  
		  else if(b2 == 1)
			memcpy(front_otp_data,front_otp_data_bank2, OTP_DATA_LEN_BYTE); // front_otp_data  
		  else if(b1 == 1)
			memcpy(front_otp_data,front_otp_data_bank1, OTP_DATA_LEN_BYTE); // front_otp_data

	   rc = fcamera_power_down(s_ctrl); //power down end
	   if(rc < 0){
  		pr_err("%s:%d camera power down failed!\n",__func__, __LINE__);
		return rc;
	   }
	   return rc;
}
// Sheldon_Li add for ov8856 --
#endif

static int rear_opt_1_read(struct seq_file *buf, void *v)
{	
	int i, rc;
	static int cnt = 0;
	/*
	for( i = 0; i < OTP_DATA_LEN_WORD; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_data_bank1[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
	}
	seq_printf(buf ,"\n");
	for( i = 0; i < OTP_DATA_LEN_WORD; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_data_bank2[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
	}
         seq_printf(buf ,"\n");
	for( i = 0; i < OTP_DATA_LEN_WORD; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_data_bank3[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
	}
	*/
#ifdef HADES_OTP_MODE
	 if(g_rear_eeprom_mapdata.mapdata != NULL){
		 for ( i = 0; i < 0x08 ;  i++){  //copy AFC(Wide)
				 rear_otp_data[i] = g_rear_eeprom_mapdata.mapdata[i];
		 }
		 for ( i = 0x08; i < 0x32 ;  i++){	//copy Module Info
				 //rear_otp_data[i-0x08] = g_rear_eeprom_mapdata.mapdata[i];
				 rear_otp_data[i] = g_rear_eeprom_mapdata.mapdata[i];
		 }	 
	}else{
			 pr_err("%s:%d failed\n",__func__, __LINE__);
	 }
#endif

	for( i = 0; i < OTP_DATA_LEN_WORD; i++)
	{
		//pr_err("Bryant %s: rear_otp_data[%d]=%x", __func__, i, rear_otp_data[i]);
		seq_printf(buf, "0x%02X ",rear_otp_data[i]);
		if((i&7) == 7)
		seq_printf(buf ,"\n");
	}

	if(cnt < 1)
	{
		//ASUS_BSP Lucien +++: Replace update ois fw after reading vcm data
		rear_otp_vcm_version = g_rear_eeprom_mapdata.mapdata[0x40A];
		pr_info("%s: Read vcm version from eeprom is 0x%02x", __func__, rear_otp_vcm_version);

		rc = ois_read_rear_vcm_version(0, rear_otp_vcm_version);
		if(rc < 0) pr_err("%s: ois_read_rear_vcm_version failed\n", __func__);
		//ASUS_BSP Lucien ---: Replace update ois fw after reading vcm data
		cnt++;
	}

	return 0;
}

static int rear_opt_2_read(struct seq_file *buf, void *v)
{	
	seq_printf(buf, "%s\n", back2_module_otp);
	/*
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_1_data_bank1[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	seq_printf(buf ,"\n");
	for( i = 0; i < OTP_DATA_LEN_WORD; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_1_data_bank2[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	seq_printf(buf ,"\n");
	for( i = 0; i < OTP_DATA_LEN_WORD; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_1_data_bank3[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	*/
#if 0
#ifdef HADES_OTP_MODE
	 if(g_rear_eeprom_mapdata.mapdata != NULL){
		 for ( i = 0x08; i < 0x10 ;  i++){    //copy AFC(Tele)
				 rear_otp_1_data[i-0x08] = g_rear_eeprom_mapdata.mapdata[i];
		 }
		
		 for ( i = 0x08; i < 0x32 ;  i++){	//copy Module Info
				 //rear_otp_1_data[i-0x08] = g_rear_eeprom_mapdata.mapdata[i];
				 rear_otp_1_data[i] = g_rear_eeprom_mapdata.mapdata[i];
		 }	 
	}else{
			 pr_err("%s:%d failed\n",__func__, __LINE__);
	 }
#endif

	for( i = 0; i < OTP_DATA_LEN_WORD; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_1_data[i]);
		if((i&7) == 7)
		seq_printf(buf ,"\n");
	}
#endif
	return 0;
}

static int front_opt_read(struct seq_file *buf, void *v)
{
	seq_printf(buf, "%s\n", front_module_otp);
#if 0
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD; i++)
	{
		seq_printf(buf, "0x%02X ",front_otp_data_bank1[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	seq_printf(buf ,"\n");
	for( i = 0; i < OTP_DATA_LEN_WORD; i++)
	{
		seq_printf(buf, "0x%02X ",front_otp_data_bank2[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	seq_printf(buf ,"\n");
	for( i = 0; i < OTP_DATA_LEN_WORD; i++)
	{
		seq_printf(buf, "0x%02X ",front_otp_data_bank3[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
#endif
	return 0;
}

  int rear_otp_1_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_opt_1_read, NULL);
}

  int rear_otp_2_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_opt_2_read, NULL);
}

  int front_otp_open(struct inode *inode, struct  file *file)
{
    return single_open(file, front_opt_read, NULL);
}

  const struct file_operations rear_1_status_fops = {
	.owner = THIS_MODULE,
	.open = rear_otp_1_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

 const struct file_operations rear_2_status_fops = {
	.owner = THIS_MODULE,
	.open = rear_otp_2_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

  const struct file_operations front_status_fops = {
	.owner = THIS_MODULE,
	.open = front_otp_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static   int rear_1_otp_bank1_proc_read(struct seq_file *buf, void *v)
{	
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD * 2; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_data_bank1[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	return 0;
}

static  int rear_2_otp_bank1_proc_read(struct seq_file *buf, void *v)
{	
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD * 2; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_1_data_bank1[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	return 0;
}
  
 static   int front_otp_bank1_proc_read(struct seq_file *buf, void *v)
{	
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD * 2; i++)
	{
		seq_printf(buf, "0x%02X ",front_otp_data_bank1[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	return 0;
}
  int rear_1_otp_bank1_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_1_otp_bank1_proc_read, NULL);
}

   int rear_2_otp_bank1_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_2_otp_bank1_proc_read, NULL);
}

   int front_otp_bank1_open(struct inode *inode, struct  file *file)
{
    return single_open(file, front_otp_bank1_proc_read, NULL);
}
  
const struct file_operations rear_1_otp_fops_bank1 = {
	.owner = THIS_MODULE,
	.open = rear_1_otp_bank1_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

const struct file_operations rear_2_otp_fops_bank1 = {
	.owner = THIS_MODULE,
	.open = rear_2_otp_bank1_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

const struct file_operations front_otp_fops_bank1 = {
	.owner = THIS_MODULE,
	.open = front_otp_bank1_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

  void create_otp_bank_1_proc_file(int cameraID)
{
	if(cameraID == CAMERA_0){
	   status_proc_file = proc_create(PROC_FILE_STATUS_BANK1_REAR_1, 0666, NULL, &rear_1_otp_fops_bank1);
	   if(status_proc_file) {
			CDBG("%s sucessed!\n", __func__);
	    } else {
			pr_err("%s failed!\n", __func__);
	    }   
	}else if(cameraID == CAMERA_1){
	    status_proc_file = proc_create(PROC_FILE_STATUS_BANK1_FRONT, 0666, NULL, &front_otp_fops_bank1);
	    if(status_proc_file) {
			CDBG("%s sucessed!\n", __func__);
	     } else {
			pr_err("%s failed!\n", __func__);
	     }   
	   
	}else if(cameraID == CAMERA_2){
	   status_proc_file = proc_create(PROC_FILE_STATUS_BANK1_REAR_2, 0666, NULL, &rear_2_otp_fops_bank1);
	   if(status_proc_file) {
			CDBG("%s sucessed!\n", __func__);
	    } else {
			pr_err("%s failed!\n", __func__);
	    }   
	}else{
		pr_err("%s failed : error camera id!\n", __func__);
	}
}

  void remove_bank1_file(void)
{
    extern struct proc_dir_entry proc_root;
    pr_info("remove_bank1_file\n");	
    remove_proc_entry(PROC_FILE_STATUS_BANK1_REAR_1, &proc_root);
    remove_proc_entry(PROC_FILE_STATUS_BANK1_REAR_2, &proc_root);
    remove_proc_entry(PROC_FILE_STATUS_BANK1_FRONT, &proc_root);
}

static  int rear_1_otp_bank2_proc_read(struct seq_file *buf, void *v)
{	
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD * 2; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_data_bank2[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	return 0;
}

static  int rear_2_otp_bank2_proc_read(struct seq_file *buf, void *v)
{	
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD * 2; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_1_data_bank2[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	return 0;
}
  
 static  int front_otp_bank2_proc_read(struct seq_file *buf, void *v)
{	
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD * 2; i++)
	{
		seq_printf(buf, "0x%02X ",front_otp_data_bank2[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	return 0;
}
  int rear_1_otp_bank2_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_1_otp_bank2_proc_read, NULL);
}

   int rear_2_otp_bank2_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_2_otp_bank2_proc_read, NULL);
}

   int front_otp_bank2_open(struct inode *inode, struct  file *file)
{
    return single_open(file, front_otp_bank2_proc_read, NULL);
}
  
const struct file_operations rear_1_otp_fops_bank2 = {
	.owner = THIS_MODULE,
	.open = rear_1_otp_bank2_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

const struct file_operations rear_2_otp_fops_bank2 = {
	.owner = THIS_MODULE,
	.open = rear_2_otp_bank2_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

const struct file_operations front_otp_fops_bank2 = {
	.owner = THIS_MODULE,
	.open = front_otp_bank2_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

  void create_otp_bank_2_proc_file(int cameraID)
{
	if(cameraID == CAMERA_0){
	   status_proc_file = proc_create(PROC_FILE_STATUS_BANK2_REAR_1, 0666, NULL, &rear_1_otp_fops_bank2);
	   if(status_proc_file) {
			CDBG("%s sucessed!\n", __func__);
	    } else {
			pr_err("%s failed!\n", __func__);
	    }   
	}else if(cameraID == CAMERA_1){
	    status_proc_file = proc_create(PROC_FILE_STATUS_BANK2_FRONT, 0666, NULL, &front_otp_fops_bank2);
	    if(status_proc_file) {
			CDBG("%s sucessed!\n", __func__);
	     } else {
			pr_err("%s failed!\n", __func__);
	     }   
	  
	}else if(cameraID == CAMERA_2){   
	   status_proc_file = proc_create(PROC_FILE_STATUS_BANK2_REAR_2, 0666, NULL, &rear_2_otp_fops_bank2);
	   if(status_proc_file) {
			CDBG("%s sucessed!\n", __func__);
	    } else {
			pr_err("%s failed!\n", __func__);
	    }   
	}else{
		pr_err("%s failed : error camera id!\n", __func__);
	}
}

  void remove_bank2_file(void)
{
    extern struct proc_dir_entry proc_root;
    pr_info("remove_bank2_file\n");	
    remove_proc_entry(PROC_FILE_STATUS_BANK2_REAR_1, &proc_root);
    remove_proc_entry(PROC_FILE_STATUS_BANK2_REAR_2, &proc_root);
    remove_proc_entry(PROC_FILE_STATUS_BANK2_FRONT, &proc_root);
}

static  int rear_1_otp_bank3_proc_read(struct seq_file *buf, void *v)
{	
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD * 2; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_data_bank3[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	return 0;
}

static  int rear_2_otp_bank3_proc_read(struct seq_file *buf, void *v)
{	
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD * 2; i++)
	{
		seq_printf(buf, "0x%02X ",rear_otp_1_data_bank3[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	return 0;
}
  
 static  int front_otp_bank3_proc_read(struct seq_file *buf, void *v)
{	
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD * 2; i++)
	{
		seq_printf(buf, "0x%02X ",front_otp_data_bank3[i]);
		if(i==7||i==15||i==23||i==31)
		seq_printf(buf ,"\n");
		else if(i==39||i==47||i==55||i==63)
		seq_printf(buf ,"\n");
	}
	return 0;
}
  int rear_1_otp_bank3_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_1_otp_bank3_proc_read, NULL);
}

   int rear_2_otp_bank3_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_2_otp_bank3_proc_read, NULL);
}

   int front_otp_bank3_open(struct inode *inode, struct  file *file)
{
    return single_open(file, front_otp_bank3_proc_read, NULL);
}
  
const struct file_operations rear_1_otp_fops_bank3 = {
	.owner = THIS_MODULE,
	.open = rear_1_otp_bank3_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

const struct file_operations rear_2_otp_fops_bank3 = {
	.owner = THIS_MODULE,
	.open = rear_2_otp_bank3_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

const struct file_operations front_otp_fops_bank3 = {
	.owner = THIS_MODULE,
	.open = front_otp_bank3_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

  void create_otp_bank_3_proc_file(int cameraID)
{
	if(cameraID == CAMERA_0){
	   status_proc_file = proc_create(PROC_FILE_STATUS_BANK3_REAR_1, 0666, NULL, &rear_1_otp_fops_bank3);
	   if(status_proc_file) {
			CDBG("%s sucessed!\n", __func__);
	    } else {
			pr_err("%s failed!\n", __func__);
	    }   
	}else if(cameraID == CAMERA_1){
	    status_proc_file = proc_create(PROC_FILE_STATUS_BANK3_FRONT, 0666, NULL, &front_otp_fops_bank3);
	    if(status_proc_file) {
			CDBG("%s sucessed!\n", __func__);
	     } else {
			pr_err("%s failed!\n", __func__);
	     }   
	}else if(cameraID == CAMERA_2){ 
            status_proc_file = proc_create(PROC_FILE_STATUS_BANK3_REAR_2, 0666, NULL, &rear_2_otp_fops_bank3);
	   if(status_proc_file) {
			CDBG("%s sucessed!\n", __func__);
	    } else {
			pr_err("%s failed!\n", __func__);
	    }   
	}else{
		pr_err("%s failed : error camera id!\n", __func__);
	}
}

  void remove_bank3_file(void)
{
    extern struct proc_dir_entry proc_root;
    pr_info("remove_bank3_file\n");	
    remove_proc_entry(PROC_FILE_STATUS_BANK3_REAR_1, &proc_root);
    remove_proc_entry(PROC_FILE_STATUS_BANK3_REAR_2, &proc_root);
    remove_proc_entry(PROC_FILE_STATUS_BANK3_FRONT, &proc_root);
}

//ASUS_BSP Lucien +++: Using service to auto update ois fw
static  int rear_otp_vcm_proc_read(struct seq_file *buf, void *v)
{
	rear_otp_vcm_version = g_rear_eeprom_mapdata.mapdata[0x40A];
	pr_info("%s: Read vcm version from eeprom is 0x%02x", __func__, rear_otp_vcm_version);
	seq_printf(buf, "0x%02X ",rear_otp_vcm_version);
	return 0;
}

int rear_otp_vcm_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_otp_vcm_proc_read, NULL);
}

const struct file_operations rear_otp_vcm= {
	.owner = THIS_MODULE,
	.open = rear_otp_vcm_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
  void create_otp_vcm_proc_file(int cameraID)
{
	if(cameraID == CAMERA_0){
	   status_proc_file = proc_create(PROC_FILE_VCM_VERSION, 0666, NULL, &rear_otp_vcm);
	   if(status_proc_file) {
			pr_info("%s sucessed!\n", __func__);
	    } else {
			pr_err("%s failed!\n", __func__);
	    }
	}
}

  void remove_vcm_file(void)
{
    extern struct proc_dir_entry proc_root;
    remove_proc_entry(PROC_FILE_VCM_VERSION, &proc_root);
}
//ASUS_BSP Lucien ---: Using service to auto update ois fw

// Sheldon_Li add for create camera otp proc file++
void create_proc_otp_thermal_file(struct msm_camera_sensor_slave_info *slave_info)
{
	   int cameraID = -1, sensorID = -1;
	   if(!slave_info){
		pr_err("%s fail : null slave_info pointer\n", __func__);
		return;
	   }
	   cameraID = slave_info->camera_id;
	   sensorID = slave_info->sensor_id_info.sensor_id;

	   //sheldon_debug("[sheldon]%s : sensorID = 0x%x, cameraID = %d\n",__func__,sensorID, cameraID);
	   //read otp file & thermal node
   	   if(sensorID==SENSOR_ID_IMX298 || sensorID==SENSOR_ID_IMX362 || sensorID==SENSOR_ID_S5K3M3 ||sensorID==SENSOR_ID_IMX214){
   		create_dump_eeprom_file(cameraID, sensorID);
   		create_thermal_file(cameraID, sensorID);
   	   }

	   if(sensorID==SENSOR_ID_IMX362 || sensorID==SENSOR_ID_S5K3M3 || sensorID==SENSOR_ID_IMX214 || sensorID == SENSOR_ID_OV8856){
		    //read common otp data
		    read_otp_asus(sensorID , cameraID);
		#if 0
			if(sensorID==SENSOR_ID_IMX362){
				pr_err("%s: SENSOR_ID_IMX362", __func__);
			} else if(sensorID==SENSOR_ID_OV8856){
				if(cameraID == CAMERA_0){
					pr_err("%s: SENSOR_ID_OV8856, CAMERA_0", __func__);
				} else if(cameraID == CAMERA_1){
					pr_err("%s: SENSOR_ID_OV8856, CAMERA_1", __func__);
				} else if(cameraID == CAMERA_2){
					pr_err("%s: SENSOR_ID_OV8856, CAMERA_2", __func__);
				}
			}
		#endif
		    //create common otp proc file
		    if(cameraID == CAMERA_0){
		   	status_proc_file = proc_create(PROC_FILE_STATUS_REAR_1, 0666, NULL, &rear_1_status_fops);
			if(status_proc_file) {
				CDBG("%s_0 sucessed!\n", __func__);
			}
			create_otp_vcm_proc_file(cameraID);
		    }
		    else if(cameraID == CAMERA_1){
			status_proc_file = proc_create(PROC_FILE_STATUS_FRONT, 0666, NULL, &front_status_fops);
			if(status_proc_file) {
				CDBG("%s_2 sucessed!\n", __func__);
			}
		   	
		    } else if(cameraID == CAMERA_2){
			status_proc_file = proc_create(PROC_FILE_STATUS_REAR_2, 0666, NULL, &rear_2_status_fops);
			if(status_proc_file) {
				CDBG("%s_1 sucessed!\n", __func__);
			}
		    } else {
				pr_err("%s fail : error camera id!\n", __func__);
		    }

		   //create 3 common otp  banks
	  	   create_otp_bank_1_proc_file(cameraID);
   	    	   create_otp_bank_2_proc_file(cameraID);
   	    	   create_otp_bank_3_proc_file(cameraID);
   	   } 
}
// Sheldon_Li add for create camera otp proc file--
  
// Sheldon_Li add for  thermal meter++
int rear_thermal_read(struct seq_file *buf, void *v)
{
	uint16_t temp=0;
	uint16_t val=0;

	  if(g_sensor_id[CAMERA_0]==SENSOR_ID_IMX298 || g_sensor_id[CAMERA_0]==SENSOR_ID_IMX362){
	  	if(g_sensor_ctrl[CAMERA_0]->sensor_state == MSM_SENSOR_POWER_UP){
			temp = sensor_read_temperature(g_sensor_id[CAMERA_0],CAMERA_0);  //temperature func
			  if(temp < 0 || temp > 0xec || temp == 0x80){
				pr_err("%s fail!\n", __func__);
				temp=0; 
				seq_printf(buf ,"%d\n",temp);
				return 0;
			  }
			  
			  if(temp < 0x50){
				val = temp;
			  } else if(temp >= 0x50 && temp < 0x80){
				val = 80;
			  }else  if(temp >= 0x81 && temp <= 0xec){
			  	val = -20;
			  }else{
			        temp -= 0xed;
			        val = temp -19;
			  }
			}else{
				temp=0;	
				seq_printf(buf ,"%d\n",temp);
				pr_debug("[camera_thermal]%s g_sensor_ctrl[CAMERA_0] not power up!\n", __func__);
				return 0;
			}
	  }else {
	  	pr_err("%s Unknow  sensor id!\n", __func__);
	  }
		
	  seq_printf(buf ,"%d\n",val*1000);
	  return 0;
  }

int rear2_thermal_read(struct seq_file *buf, void *v)
  {   
         uint16_t temp=0;
	uint16_t val=0;

	  if(g_sensor_id[CAMERA_2]==SENSOR_ID_IMX362){
	  	if(g_sensor_ctrl[CAMERA_2]->sensor_state == MSM_SENSOR_POWER_UP){
			 temp = sensor_read_temperature(g_sensor_id[CAMERA_2],CAMERA_2); //temperature func
			  if(temp < 0 || temp > 0xec || temp == 0x80){
				pr_err("%s fail!\n", __func__);
				temp=0; 
				seq_printf(buf ,"%d\n",temp);
				return 0;
			  }
			  
			  if(temp < 0x50){
				val = temp;
			  } else if(temp >= 0x50 && temp < 0x80){
				val = 80;
			  }else  if(temp >= 0x81 && temp <= 0xec){
			  	val = -20;
			  }else{
			        temp -= 0xed;
			        val = temp -19;
			  }
			}else{
				temp=0;	
				seq_printf(buf ,"%d\n",temp);
				pr_debug("[camera_thermal]%s g_sensor_ctrl[CAMERA_0] not power up!\n", __func__);
				return 0;
			}
	  }else {
	  	pr_err("%s Unknow  sensor id!\n", __func__);
	  }
		
	  seq_printf(buf ,"%d\n",val*1000);
	  return 0;
  }

  
int front_thermal_read(struct seq_file *buf, void *v)
  {
	 uint16_t temp=0;
	 uint16_t val=0;

	if(g_sensor_id[CAMERA_1]==SENSOR_ID_IMX298 || g_sensor_id[CAMERA_1]==SENSOR_ID_IMX214){
	  	if(g_sensor_ctrl[CAMERA_1]->sensor_state == MSM_SENSOR_POWER_UP){
		  temp = sensor_read_temperature(g_sensor_id[CAMERA_1], CAMERA_1); //temperature func
		  if(temp < 0 || temp > 0xec || temp == 0x80){
			pr_err("%s fail!\n", __func__);
			temp=0; 
			seq_printf(buf ,"%d\n",temp);
			return 0;
		  }
		  
		  if(temp < 0x50){
			val = temp;
		  } else if(temp >= 0x50 && temp < 0x80){
			val = 80;
		  }else  if(temp >= 0x81 && temp <= 0xec){
		  	val = -20;
		  }else{
		        temp -= 0xed;
		        val = temp -19;
		  }
	  	}else{
				temp=0;	
				seq_printf(buf ,"%d\n",temp);
				pr_debug("[camera_thermal]%s g_sensor_ctrl[CAMERA_1] not power up!\n", __func__);
				return 0;
		}
	  }else {
	  	pr_err("%s Unknow  sensor id!\n", __func__);
	  }
		
	  seq_printf(buf ,"%d\n",val*1000);
	  return 0;
  }
  
int rear_thermal_file_open(struct inode *inode, struct  file *file)
  {
	  return single_open(file, rear_thermal_read, NULL);
  }

int rear2_thermal_file_open(struct inode *inode, struct  file *file)
  {
	  return single_open(file, rear2_thermal_read, NULL);
  }

  
int front_thermal_file_open(struct inode *inode, struct  file *file)
  {
	  return single_open(file, front_thermal_read, NULL);
  }
  
 const struct file_operations rear_thermal_fops = {
	  .owner = THIS_MODULE,
	  .open = rear_thermal_file_open,
	  .read = seq_read,
	  .llseek = seq_lseek,
	  .release = single_release,
  };
 
 const struct file_operations rear2_thermal_fops = {
	  .owner = THIS_MODULE,
	  .open = rear2_thermal_file_open,
	  .read = seq_read,
	  .llseek = seq_lseek,
	  .release = single_release,
  };
  
const struct file_operations front_thermal_fops = {
	  .owner = THIS_MODULE,
	  .open = front_thermal_file_open,
	  .read = seq_read,
	  .llseek = seq_lseek,
	  .release = single_release,
  };


//ASUS_BSP_Sheldon +++ "read eeprom thermal data for DIT"
int float32Toint24(int addr, int len)
{
    int i=0;
    int retVal=0;
    uint dataVal=0;
    uint negative,exponent,mantissa;
    u8 rawData[5] = {0};
   
	if(g_rear_eeprom_mapdata.mapdata != NULL)
	{
	    if(addr > g_rear_eeprom_mapdata.num_data)
	    {
	    	pr_err("sz_cam_%s: Address out of range!\n",__func__);
		return 0;
	    }
		
	    for (i = 0; i < len;  i++)
	    {
	       rawData[i] = g_rear_eeprom_mapdata.mapdata[addr+len-i-1];
	    }
	    memcpy(&dataVal,rawData,len);

	    negative = ((dataVal >> 31) & 0x1);      // 0x10000000
    	    exponent = ((dataVal >> 23) & 0xFF);  // 0x7F800000
    	    mantissa = ((dataVal >>  0) & 0x7FFFFF) | 0x800000 ; // 0x007FFFFF implicit bit
             retVal = mantissa >> (22 - (exponent - 0x80));   
	}else{
		pr_err("sz_cam_%s: g_rear_eeprom_mapdata.mapdata is NULL!\n",__func__);
	}
         if( !exponent )
         	 return 0;
    	if( negative )
        		 return -retVal;
         else
         	 return  retVal;
}

static int rear_eeprom_temp(struct seq_file *buf, void *v)
{
        //AFC(Wide) :
        //Infinity temperature
         seq_printf(buf, "%d ",float32Toint24(0x1039,4));
        //Middle temperature
         seq_printf(buf, "%d ",float32Toint24(0x103D,4));
        //Macro temperature
         seq_printf(buf, "%d\n",float32Toint24(0x1041,4));
	return 0;
}

  int rear_eeprom_temp_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_eeprom_temp, NULL);
}

const struct file_operations rear_eeprom_temp_fops = {
	.owner = THIS_MODULE,
	.open = rear_eeprom_temp_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int rear2_eeprom_temp(struct seq_file *buf, void *v)
{	
        //AFC(Tele):
        //Infinity temperature
         seq_printf(buf, "%d ",float32Toint24(0x1045,4));
        //Middle temperature
         seq_printf(buf, "%d ",float32Toint24(0x1049,4));
       //Macro temperature
         seq_printf(buf, "%d\n",float32Toint24(0x104D,4));
	return 0;
}

  int rear2_eeprom_temp_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear2_eeprom_temp, NULL);
}

const struct file_operations rear2_eeprom_temp_fops = {
	.owner = THIS_MODULE,
	.open = rear2_eeprom_temp_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

//ASUS_BSP_Sheldon --- "read eeprom thermal data for DIT"
void create_thermal_file(int camID, int sensorID)
  {
         if(camID == 0){
	  if(sensorID == SENSOR_ID_IMX298 || sensorID == SENSOR_ID_IMX362){
	  	   //for thermal team=======================
	  	   status_proc_file = proc_create(PROC_FILE_THERMAL_REAR, 0666, NULL, &rear_thermal_fops);
		   if(status_proc_file) {
			  CDBG("%s 0 sucessed!\n", __func__);
		   } else {
			  pr_err("%s 0 failed!\n", __func__);
		   }   
		   
		   //for DIT===============================
		   status_proc_file = proc_create(PROC_FILE_THERMAL_WIDE, 0666, NULL, &rear_thermal_fops);
		   if(status_proc_file) {
			  CDBG("%s wide sucessed!\n", __func__);
		   } else {
			  pr_err("%s wide failed!\n", __func__);
		   }   

		   status_proc_file = proc_create(PROC_FILE_EEPEOM_THERMAL_WIDE, 0666, NULL, &rear_eeprom_temp_fops);
		   if(status_proc_file) {
			  CDBG("%s eeprom wide sucessed!\n", __func__);
		   } else {
			  pr_err("%s eeprom wide failed!\n", __func__);
		   }   
	  }
	}
 	else if(camID == 1){
	  if(sensorID == SENSOR_ID_IMX298 || sensorID == SENSOR_ID_IMX214){
	  	 status_proc_file = proc_create(PROC_FILE_THERMAL_FRONT, 0666, NULL, &front_thermal_fops);
		  if(status_proc_file) {
		  	CDBG("%s 1 sucessed!\n", __func__);
		   } else {
			  pr_err("%s 1 failed!\n", __func__);
		   }
	  }
	}
	else if(camID == 2){
	  if(sensorID == SENSOR_ID_S5K3M3){
	  	   //for DIT===============================
		   status_proc_file = proc_create(PROC_FILE_THERMAL_TELE, 0666, NULL, &rear_thermal_fops);
		   if(status_proc_file) {
			  CDBG("%s tele sucessed!\n", __func__);
		   } else {
			  pr_err("%s tele failed!\n", __func__);
		   }   
		   
		   status_proc_file = proc_create(PROC_FILE_EEPEOM_THERMAL_TELE, 0666, NULL, &rear2_eeprom_temp_fops);
		   if(status_proc_file) {
			  CDBG("%s eeprom tele sucessed!\n", __func__);
		   } else {
			  pr_err("%s eeprom tele failed!\n", __func__);
		   }   
	  }  
	}
  }
// Sheldon_Li add for  thermal meter--

 void remove_thermal_file(void)
  {
	  extern struct proc_dir_entry proc_root;
	  pr_info("sz_cam_remove_thermal_file\n");   
	  remove_proc_entry(PROC_FILE_THERMAL_REAR, &proc_root);
	  remove_proc_entry(PROC_FILE_THERMAL_FRONT, &proc_root);
	  remove_proc_entry(PROC_FILE_THERMAL_WIDE,&proc_root);
	  remove_proc_entry(PROC_FILE_THERMAL_TELE,&proc_root);
	  remove_proc_entry(PROC_FILE_EEPEOM_THERMAL_WIDE,&proc_root);
	  remove_proc_entry(PROC_FILE_EEPEOM_THERMAL_TELE,&proc_root);
  }


  // Sheldon_Li add for  eeprom map to OTP++
  int rear_eeprom_dump(struct seq_file *buf, void *v)
{	
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD*3; i++)
	{
	        if(g_rear_eeprom_mapdata.mapdata != NULL){
			if( i != 0 && (i%32 == 0))
			seq_printf(buf ,"\n");	
			seq_printf(buf, "0x%02X ",g_rear_eeprom_mapdata.mapdata[i]);
			if((i&7) == 7)
			seq_printf(buf ,"\n");
			
		}else{
			seq_printf(buf ,"No rear EEprom!\n");
			break;
		}
		
	}
	return 0;
}

  int front_eeprom_dump(struct seq_file *buf, void *v)
{
	int i;
	for( i = 0; i < OTP_DATA_LEN_WORD*3; i++)
	{
		if(g_front_eeprom_mapdata.mapdata != NULL){
			if( i != 0 && (i%32 == 0))
			seq_printf(buf ,"\n");	
			seq_printf(buf, "0x%02X ",g_front_eeprom_mapdata.mapdata[i]);
			if((i&7) == 7)
			seq_printf(buf ,"\n");
		}else{
			seq_printf(buf ,"No front EEprom!\n");
			break;
		}
	}
	return 0;
}

  int rear_eeprom_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_eeprom_dump, NULL);
}

  int front_eeprom_open(struct inode *inode, struct  file *file)
{
    return single_open(file, front_eeprom_dump, NULL);
}

  const struct file_operations rear_eeprom_fops = {
	.owner = THIS_MODULE,
	.open = rear_eeprom_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

  const struct file_operations front_eeprom_fops = {
	.owner = THIS_MODULE,
	.open = front_eeprom_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

//ASUS_BSP_Sheldon +++ "read ArcSoft Dual Calibration Data for DIT"
#define ARCSOFTDATA_START 0x0834
#define ARCSOFTDATA_END    0x1034
static int read_eeprom_arcsoft_dualCali(struct seq_file *buf, void *v)
{
	//ArcSoft Dual Calibration Data
	int i=0;
	if(g_rear_eeprom_mapdata.mapdata != NULL)
	{
		 for ( i = ARCSOFTDATA_START; i < ARCSOFTDATA_END && i < g_rear_eeprom_mapdata.num_data;  i++)
		 {
			seq_printf(buf, "%c",g_rear_eeprom_mapdata.mapdata[i]);	
		 }
	}else{
	  	pr_err("sz_cam_%s: g_rear_eeprom_mapdata.mapdata is NULL!\n",__func__);
	}
	return 0;
}

int eeprom_arcsoft_dualCali_open(struct inode *inode, struct  file *file)
{
	return single_open(file, read_eeprom_arcsoft_dualCali, NULL);
}

const struct file_operations eeprom_arcsoft_dualCali_fops = {
	.owner = THIS_MODULE,
	.open = eeprom_arcsoft_dualCali_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
//ASUS_BSP_Sheldon --- "read ArcSoft Dual Calibration Data for DIT"
	
void create_dump_eeprom_file(int camID, int sensorID)
{
	   if(camID == 0){
		if(sensorID == SENSOR_ID_IMX362){
			status_proc_file = proc_create(PROC_FILE_EEPEOM_ARCSOFT, 0666, NULL, &eeprom_arcsoft_dualCali_fops);
		}
	   	else if(sensorID == SENSOR_ID_IMX298){
			 status_proc_file = proc_create(PROC_FILE_STATUS_REAR_1, 0666, NULL, &rear_eeprom_fops);
		}
		if(status_proc_file) {
			CDBG("%s sucessed!\n", __func__);
	   	 } else {
			pr_err("%s failed!\n", __func__);
	    	}   

	   }else if(camID == 1){
	   	if(sensorID == SENSOR_ID_IMX298){
			status_proc_file = proc_create(PROC_FILE_STATUS_FRONT, 0666, NULL, &front_eeprom_fops);
	   	}
		if(status_proc_file) {
			CDBG("%s sucessed!\n", __func__);
	    	} else {
			pr_err("%s failed!\n", __func__);
	    	}
	 }
}
  // Sheldon_Li add for  eeprom map to OTP--

  int pdaf_info_proc_read(struct seq_file *buf, void *v)
  {
	  seq_printf(buf, "%s",pdaf_info);
	  seq_printf(buf ,"\n");
	  return 0;
  }

  ssize_t pdaf_info_proc_file_read (struct file *filp, char __user *buff, size_t size, loff_t *ppos){
	int ret;
	int count;
	count = copy_to_user(buff, pdaf_info, size);
	if(copy_to_user(buff, pdaf_info, size)){
		ret =  - EFAULT;
	}else{
		*ppos += count;
		ret = count;
	}
	return ret;
	}
  ssize_t pdaf_info_proc_file_write (struct file *filp, const char __user *buff, size_t size, loff_t *ppos){
	  int ret;
	  memset(pdaf_info, 0, PDAF_INFO_MAX_SIZE);
	  if(copy_from_user(pdaf_info, buff, size)){
		  ret =  - EFAULT;
		  pr_err("%s copy from us failed!\n", __func__);
		  }else{
		  *ppos += size;
		  ret = size;
		  }
	  return ret;
	}

  int pdaf_info_proc_open(struct inode *inode, struct  file *file)
  {
	  return single_open(file, pdaf_info_proc_read, NULL);
  }

  //rear and front use same ops
  const struct file_operations pdaf_info_fops = {
	  .owner = THIS_MODULE,
	  .open = pdaf_info_proc_open,
	  .read = seq_read,
	  //.read = pdaf_info_proc_file_read,
	  .write = pdaf_info_proc_file_write,
	  .llseek = seq_lseek,
	  .release = single_release,
  };

  void create_proc_pdaf_info(void)
  {
	if(!pdaf_info){
	  pdaf_info = kmalloc(PDAF_INFO_MAX_SIZE, GFP_KERNEL);
	  memset(pdaf_info,0,PDAF_INFO_MAX_SIZE);
	  pdaf_file = proc_create(REAR_PROC_FILE_PDAF_INFO, 0666, NULL, &pdaf_info_fops);
	  if(pdaf_file) {
		  CDBG("%s sucessed!\n", __func__);
	  } else {
		  pr_err("%s failed!\n", __func__);
	  }

	  pdaf_file = proc_create(FRONT_PROC_FILE_PDAF_INFO, 0666, NULL, &pdaf_info_fops);
	  if(pdaf_file) {
		  CDBG("%s sucessed!\n", __func__);
	  } else {
		  pr_err("%s failed!\n", __func__);
	  }
	}
  }

/*
	clear_proc_pdaf_info()
	Clear the pdaf data of node when close camera.
*/
void clear_proc_pdaf_info(void){
	memset(pdaf_info,0,PDAF_INFO_MAX_SIZE);
}
  void remove_file(void)
{
    extern struct proc_dir_entry proc_root;
    pr_info("remove_file\n");	
    remove_proc_entry(PROC_FILE_STATUS_REAR_1, &proc_root);
    remove_proc_entry(PROC_FILE_STATUS_REAR_2, &proc_root);
    remove_proc_entry(PROC_FILE_STATUS_FRONT, &proc_root);
    remove_proc_entry(REAR_PROC_FILE_PDAF_INFO,&proc_root);
    remove_proc_entry(FRONT_PROC_FILE_PDAF_INFO,&proc_root);
    remove_proc_entry(PROC_FILE_EEPEOM_ARCSOFT,&proc_root);
}

  int rear_1_module_proc_read(struct seq_file *buf, void *v)
{
	  if(g_sensor_id[CAMERA_0]==SENSOR_ID_IMX362){
			seq_printf(buf,"IMX362\n");
	  }else if(g_sensor_id[CAMERA_0]==SENSOR_ID_S5K3M3){
		  seq_printf(buf,"S5K3M3\n");
	  }else if(g_sensor_id[CAMERA_0]==SENSOR_ID_IMX214){
		  seq_printf(buf,"IMX214\n");
	  }else if(g_sensor_id[CAMERA_0]==SENSOR_ID_IMX298){
		  seq_printf(buf,"IMX298\n");
	  }else if(g_sensor_id[CAMERA_0]==SENSOR_ID_OV5670){
		  seq_printf(buf,"OV5670\n");
	  }else if(g_sensor_id[CAMERA_0]==SENSOR_ID_OV8856){
	           seq_printf(buf,"OV8856\n");
	  }
	  return 0;
}
  
  int rear_2_module_proc_read(struct seq_file *buf, void *v)
  {
	  if(g_sensor_id[CAMERA_2]==SENSOR_ID_IMX351){
		   seq_printf(buf,"IMX351\n");
         }else if(g_sensor_id[CAMERA_2]==SENSOR_ID_S5K3M3){
		   seq_printf(buf,"S5K3M3\n");
	  }else if(g_sensor_id[CAMERA_2]==SENSOR_ID_IMX214){
		   seq_printf(buf,"IMX214\n");
	  }else if(g_sensor_id[CAMERA_2]==SENSOR_ID_IMX298){
		   seq_printf(buf,"IMX298\n");
	  }else if(g_sensor_id[CAMERA_2]==SENSOR_ID_OV5670){
		   seq_printf(buf,"OV5670\n");
	  }else if(g_sensor_id[CAMERA_2]==SENSOR_ID_OV8856){
	            seq_printf(buf,"OV8856\n");
	  }
	  return 0;
  }

  int front_module_proc_read(struct seq_file *buf, void *v)
{
	if(g_sensor_id[CAMERA_1]==SENSOR_ID_IMX214){
		   seq_printf(buf,"IMX214\n");
	  }else if(g_sensor_id[CAMERA_1]==SENSOR_ID_IMX319){
		   seq_printf(buf,"IMX319\n");
	  }else if(g_sensor_id[CAMERA_1]==SENSOR_ID_S5K3M3){
		   seq_printf(buf,"S5K3M3\n");
	  }else if(g_sensor_id[CAMERA_1]==SENSOR_ID_IMX298){
		   seq_printf(buf,"IMX298\n");
	  }else if(g_sensor_id[CAMERA_1]==SENSOR_ID_OV5670){
		   seq_printf(buf,"OV5670\n");
	  }else if(g_sensor_id[CAMERA_1]==SENSOR_ID_OV8856){
	            seq_printf(buf,"OV8856\n");
	  }
	  return 0;
}

  int rear_1_module_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_1_module_proc_read, NULL);
}

    int rear_2_module_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, rear_2_module_proc_read, NULL);
}

  int front_module_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, front_module_proc_read, NULL);
}

  const struct file_operations rear_1_module_fops = {
	.owner = THIS_MODULE,
	.open = rear_1_module_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

    const struct file_operations rear_2_module_fops = {
	.owner = THIS_MODULE,
	.open = rear_2_module_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

  const struct file_operations front_module_fops = {
	.owner = THIS_MODULE,
	.open = front_module_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

  void create_rear_module_proc_file(int cameraID)
{
	if(cameraID == CAMERA_0){
		  status_proc_file = proc_create(PROC_FILE_REARMODULE_1, 0666, NULL, &rear_1_module_fops);
		   if(status_proc_file) {
				CDBG("%s_1  sucessed!\n", __func__);
		    } else {
				pr_err("%s_1 failed!\n", __func__);
	  	   }   
	}else if(cameraID == CAMERA_2){
		  status_proc_file = proc_create(PROC_FILE_REARMODULE_2, 0666, NULL, &rear_2_module_fops);
		   if(status_proc_file) {
				CDBG("%s_2 sucessed!\n", __func__);
		    } else {
				pr_err("%s_2 failed!\n", __func__);
	  	   }   
	}
}

  void create_front_module_proc_file(void)
{
	status_proc_file = proc_create(PROC_FILE_FRONTMODULE, 0666, NULL, &front_module_fops);
	
	if(status_proc_file) {
		CDBG("%s sucessed!\n", __func__);
   	 } else {
		pr_err("%s failed!\n", __func__);
        }
}

  void remove_module_file(void)
{
    extern struct proc_dir_entry proc_root;
    pr_info("remove_module_file\n");	
    remove_proc_entry(PROC_FILE_REARMODULE_1, &proc_root);
    remove_proc_entry(PROC_FILE_REARMODULE_2, &proc_root);
    remove_proc_entry(PROC_FILE_FRONTMODULE, &proc_root);
}

//static DEVICE_ATTR(camera_status, S_IRUGO | S_IWUSR | S_IWGRP ,rear_status_proc_read,NULL);
//static DEVICE_ATTR(vga_status, S_IRUGO | S_IWUSR | S_IWGRP ,front_status_proc_read,NULL);
//static DEVICE_ATTR(camera_resolution, S_IRUGO | S_IWUSR | S_IWGRP ,rear_resolution_read,NULL);
//static DEVICE_ATTR(vga_resolution, S_IRUGO | S_IWUSR | S_IWGRP ,front_resolution_read,NULL);

struct kobject *camera_status_camera_1;
EXPORT_SYMBOL_GPL(camera_status_camera_1);
struct kobject *camera_status_camera_2;
EXPORT_SYMBOL_GPL(camera_status_camera_2);
struct kobject *camera_status_vga;
EXPORT_SYMBOL_GPL(camera_status_vga);

static const struct device_attribute dev_attr_camera_1_status = 
	__ATTR(camera_status, S_IRUGO | S_IWUSR | S_IWGRP, rear_1_status_proc_read, NULL);
static const struct device_attribute dev_attr_camera_2_status = 
	__ATTR(camera_status_2, S_IRUGO | S_IWUSR | S_IWGRP, rear_2_status_proc_read, NULL);
static const struct device_attribute dev_attr_vga_status =
	__ATTR(vga_status, S_IRUGO | S_IWUSR | S_IWGRP, front_status_proc_read, NULL);

	
 ssize_t rear_1_status_proc_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	  struct msm_sensor_ctrl_t *s_ctrl = g_sensor_ctrl[CAMERA_0];

		  if(s_ctrl != NULL)
		  {
			  if((msm_sensor_testi2c(s_ctrl))<0)
			  return sprintf(buf, "%s\n%s\n", "ERROR: i2c r/w test fail","Driver version:");
			  else
			  return sprintf(buf, "%s\n%s\n%s0x%x\n", "ACK:i2c r/w test ok","Driver version:","sensor_id:",g_sensor_id[CAMERA_0]);	  
		  }
		  else{
		  	 return sprintf(buf, "%s\n%s\n", "ERROR: i2c r/w test fail","Driver version:");
		  }  
}

 ssize_t rear_2_status_proc_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	  struct msm_sensor_ctrl_t *s_ctrl = g_sensor_ctrl[CAMERA_2];

		  if(s_ctrl != NULL)
		  {
			  if((msm_sensor_testi2c(s_ctrl))<0)
			  return sprintf(buf, "%s\n%s\n", "ERROR: i2c r/w test fail","Driver version:");
			  else
			  return sprintf(buf, "%s\n%s\n%s0x%x\n", "ACK:i2c r/w test ok","Driver version:","sensor_id:",g_sensor_id[CAMERA_2]);	  
		  }
		  else{
		  	 return sprintf(buf, "%s\n%s\n", "ERROR: i2c r/w test fail","Driver version:");
		  }  
}
 ssize_t front_status_proc_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	  struct msm_sensor_ctrl_t *s_ctrl = g_sensor_ctrl[CAMERA_1];
	  
		  if(s_ctrl != NULL)
		  {
			   if((msm_sensor_testi2c(s_ctrl))<0)
			   return sprintf(buf, "%s\n%s\n", "ERROR: i2c r/w test fail","Driver version:");
			   else
			   return sprintf(buf, "%s\n%s\n%s0x%x\n", "ACK:i2c r/w test ok","Driver version:","sensor_id:",g_sensor_id[CAMERA_1]);   
		  }
		   else{
		   	  return sprintf(buf, "%s\n%s\n", "ERROR: i2c r/w test fail","Driver version:");
		   }
}

  int create_rear_status_proc_file(int cameraID)
{	
	int ret;
	   if(cameraID == CAMERA_0){
	   	camera_status_camera_1 = kobject_create_and_add("camera_status_camera", NULL);
		if (!camera_status_camera_1)
			return -ENOMEM;
		ret = sysfs_create_file(camera_status_camera_1, &dev_attr_camera_1_status.attr);
		if (ret)
		{
			printk("%s: sysfs_create failed\n", __func__);
			return ret;
		}
	   }else if(cameraID == CAMERA_2){
	   	camera_status_camera_2 = kobject_create_and_add("camera_status_camera_2", NULL);
		if (!camera_status_camera_2)
			return -ENOMEM;
		ret = sysfs_create_file(camera_status_camera_2, &dev_attr_camera_2_status.attr);
		if (ret)
		{
			printk("%s: sysfs_create failed\n", __func__);
			return ret;
		}
	   }
	   return 0;
}
  int create_front_status_proc_file(void)
{	
	int ret;
		camera_status_vga = kobject_create_and_add("camera_status_vga", NULL);
		if (!camera_status_vga)
			return -ENOMEM;
		ret = sysfs_create_file(camera_status_vga, &dev_attr_vga_status.attr);
		if (ret)
		{
			printk("%s: sysfs_create failed\n", __func__);
			return ret;
		}
		return 0;
}
  void remove_proc_file(void)
{
	sysfs_remove_file(camera_status_camera_1, &dev_attr_camera_1_status.attr);
	kobject_del(camera_status_camera_1);
	sysfs_remove_file(camera_status_camera_2, &dev_attr_camera_2_status.attr);
	kobject_del(camera_status_camera_2);
	sysfs_remove_file(camera_status_vga, &dev_attr_vga_status.attr);
	kobject_del(camera_status_vga);
}

 ssize_t rear_1_resolution_read(struct device *dev, struct device_attribute *attr, char *buf)
  {
	  if(g_sensor_id[CAMERA_0]==SENSOR_ID_IMX362)
		  return sprintf(buf,"12M\n");
	  else if(g_sensor_id[CAMERA_0]==SENSOR_ID_S5K3M3)
		  return sprintf(buf,"13M\n");
	  else if(g_sensor_id[CAMERA_0]==SENSOR_ID_IMX214)
		  return sprintf(buf,"13M\n");
	  else if(g_sensor_id[CAMERA_0]==SENSOR_ID_IMX298)
		  return sprintf(buf,"16M\n");
	  else
		  return sprintf(buf,"None\n");
  }

 ssize_t rear_2_resolution_read(struct device *dev, struct device_attribute *attr, char *buf)
  {
	   if(g_sensor_id[CAMERA_2]==SENSOR_ID_IMX362)
		  return sprintf(buf,"12M\n");
	  else if(g_sensor_id[CAMERA_2]==SENSOR_ID_S5K3M3)
		  return sprintf(buf,"13M\n");
	  else if(g_sensor_id[CAMERA_2]==SENSOR_ID_IMX214)
		  return sprintf(buf,"13M\n");
	  else if(g_sensor_id[CAMERA_2]==SENSOR_ID_IMX298)
		  return sprintf(buf,"16M\n");
	  else
		  return sprintf(buf,"None\n");
  }

 ssize_t front_resolution_read(struct device *dev, struct device_attribute *attr, char *buf)
  {
  	  if(g_sensor_id[CAMERA_1]==SENSOR_ID_IMX214)
		  return sprintf(buf,"13M\n");
	  else if(g_sensor_id[CAMERA_1]==SENSOR_ID_IMX362)
		  return sprintf(buf,"12M\n");
	  else if(g_sensor_id[CAMERA_1]==SENSOR_ID_S5K3M3)
		  return sprintf(buf,"13M\n");
	  else if(g_sensor_id[CAMERA_1]==SENSOR_ID_OV8856)
		  return sprintf(buf,"8M\n");
	  else if(g_sensor_id[CAMERA_1]==SENSOR_ID_IMX319)
		  return sprintf(buf,"8M\n");
	  else
		  return sprintf(buf,"None\n");
  }

struct kobject *camera_resolution_camera_1;
EXPORT_SYMBOL_GPL(camera_resolution_camera_1);
struct kobject *camera_resolution_camera_2;
EXPORT_SYMBOL_GPL(camera_resolution_camera_2);
struct kobject *camera_resolution_vga;
EXPORT_SYMBOL_GPL(camera_resolution_vga);

static const struct device_attribute dev_attr_camera_1_resolution =
	__ATTR(camera_resolution, S_IRUGO | S_IWUSR | S_IWGRP, rear_1_resolution_read, NULL);  
static const struct device_attribute dev_attr_camera_2_resolution =
	__ATTR(camera_2_resolution, S_IRUGO | S_IWUSR | S_IWGRP, rear_2_resolution_read, NULL);  
static const struct device_attribute dev_attr_vga_resolution =
	__ATTR(vga_resolution, S_IRUGO | S_IWUSR | S_IWGRP, front_resolution_read, NULL);

  int create_rear_resolution_proc_file(int cameraID)
{
	   int ret;
		
	   if(cameraID == CAMERA_0){
	   	camera_resolution_camera_1 = kobject_create_and_add("camera_resolution_camera", NULL);
		if (!camera_resolution_camera_1)
			return -ENOMEM;
		
		ret = sysfs_create_file(camera_resolution_camera_1, &dev_attr_camera_1_resolution.attr);
		if (ret)
		{
			printk("%s:_1 sysfs_create failed\n", __func__);
			return ret;
		}
	   }else if(cameraID == CAMERA_2){
	   	camera_resolution_camera_2 = kobject_create_and_add("camera_resolution_camera_2", NULL);
		if (!camera_resolution_camera_2)
			return -ENOMEM;
		
	  	ret = sysfs_create_file(camera_resolution_camera_2, &dev_attr_camera_2_resolution.attr);
		if (ret)
		{
			printk("%s:_2 sysfs_create failed\n", __func__);
			return ret;
		}
	   }
	   return 0;
}
  
  int create_front_resolution_proc_file(void)
{	
		int ret;
			camera_resolution_vga = kobject_create_and_add("camera_resolution_vga", NULL);
			if (!camera_resolution_vga)
				return -ENOMEM;
			ret = sysfs_create_file(camera_resolution_vga, &dev_attr_vga_resolution.attr);
			if (ret)
			{
				printk("%s: sysfs_create failed\n", __func__);
				return ret;
			}
			return 0;
}
  
  void remove_resolution_file(void)
{
	sysfs_remove_file(camera_resolution_camera_1, &dev_attr_camera_1_resolution.attr);
	kobject_del(camera_resolution_camera_1);
	sysfs_remove_file(camera_resolution_camera_2, &dev_attr_camera_2_resolution.attr);
	kobject_del(camera_resolution_camera_2);
	sysfs_remove_file(camera_resolution_vga, &dev_attr_vga_resolution.attr);
	kobject_del(camera_resolution_vga);
}


