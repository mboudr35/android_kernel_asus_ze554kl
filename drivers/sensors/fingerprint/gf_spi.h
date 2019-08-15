#ifndef __GF_SPI_H
#define __GF_SPI_H

#include <linux/types.h>
#include <linux/notifier.h>
/**********************************************************/
enum FP_MODE{
	GF_IMAGE_MODE = 0,
	GF_KEY_MODE,
	GF_SLEEP_MODE,
	GF_FF_MODE,
	GF_DEBUG_MODE = 0x56
};

struct gf_key {
	unsigned int key;
	int value;
};

struct gf_key_map
{
    char *name;
    unsigned short val;
	unsigned short report_val;
};

#define  GF_IOC_MAGIC         'G'
#define  GF_IOC_DISABLE_IRQ	_IO(GF_IOC_MAGIC, 0)
#define  GF_IOC_ENABLE_IRQ	_IO(GF_IOC_MAGIC, 1)
#define  GF_IOC_SETSPEED    _IOW(GF_IOC_MAGIC, 2, unsigned int)
#define  GF_IOC_RESET       _IO(GF_IOC_MAGIC, 3)
#define  GF_IOC_COOLBOOT    _IO(GF_IOC_MAGIC, 4)
#define  GF_IOC_SENDKEY    _IOW(GF_IOC_MAGIC, 5, struct gf_key)
#define  GF_IOC_CLK_READY  _IO(GF_IOC_MAGIC, 6)
#define  GF_IOC_CLK_UNREADY  _IO(GF_IOC_MAGIC, 7)
#define  GF_IOC_PM_FBCABCK  _IO(GF_IOC_MAGIC, 8)
#define  GF_IOC_POWER_ON   _IO(GF_IOC_MAGIC, 9)
#define  GF_IOC_POWER_OFF  _IO(GF_IOC_MAGIC, 10)
#define  GF_IOC_TOUCH_ENABLE_MASK  _IO(GF_IOC_MAGIC, 14)
#define  GF_IOC_TOUCH_DISABLE_MASK  _IO(GF_IOC_MAGIC, 13)
#define  GF_IOC_CLEAN_EARLY_WAKE_HINT  _IO(GF_IOC_MAGIC, 15)

#define  GF_IOC_MAXNR    16

/*#define AP_CONTROL_CLK       1 */
#define  USE_PLATFORM_BUS    1
/*#define  USE_SPI_BUS	       1 */

#define GF_FASYNC   1	/*If support fasync mechanism.*/

#define GF_EARLY_WAKE   1	/*If support early wake up mechanism.*/

#define GF_NETLINK_ENABLE 1
#define GF_NET_EVENT_IRQ 0
#define GF_NET_EVENT_FB_BLACK 1
#define GF_NET_EVENT_FB_UNBLACK 2
#define GF_DEFAULT_SPEED 1000000

/*
int gf_parse_dts(struct fp_device_data* gf_dev);
void gf_cleanup(struct fp_device_data *gf_dev);

int gf_power_on(struct fp_device_data *gf_dev);
int gf_power_off(struct fp_device_data *gf_dev);

int gf_hw_reset(struct fp_device_data *gf_dev, unsigned int delay_ms);
int gf_irq_num(struct fp_device_data *gf_dev);
*/
void sendnlmsg(char *message);
int netlink_init(void);
void netlink_exit(void);

#endif /*__GF_SPI_H*/
