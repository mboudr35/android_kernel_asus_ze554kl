#include <vfsSpiDrv.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <asm/uaccess.h>


#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <linux/notifier.h>

#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/timer.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/slab.h>

#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/i2c/twl.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/irq.h>
#include <linux/compat.h>

#include <asm-generic/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/jiffies.h>

#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>

#include <linux/pinctrl/consumer.h>
#include <linux/clk.h>
#include <soc/qcom/scm.h>
#include <soc/qcom/qseecomi.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
//herman poll wait start
#include <linux/poll.h>
//herman poll wait end

// goodix +++
/*
#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif
*/
#include <linux/ktime.h>
#include <linux/pm_qos.h>
#include <linux/cpufreq.h>
#include "gf_spi.h"
#include <linux/platform_device.h>
// goodix ---

#include <linux/wakelock.h>

#include <linux/completion.h>
#define reinit_completion(x) INIT_COMPLETION(*(x))

static int g_module_vendor;
#define vendor_module_syna 1
#define vendor_module_gdix_316m 2
#define vendor_module_gdix_5116m 3
#define vendor_module_gdix_3266A 4
#define vendor_module_gdix_5206 5
#define vendor_module_gdix_5216 6

extern bool g_Charger_mode;
bool g_FP_Disable_Touch = false;
extern u32 g_update_bl;


struct fp_device_data {
	/* +++ common part +++ */
	struct regulator *sovcc;
	struct regulator *vcc;
	unsigned int drdy_pin; // int_gpio
	unsigned int isr_pin;	// isr
	unsigned int sleep_pin; // rst_gpio
	unsigned int osvcc_Titan_enable_pin;
	unsigned int module_vendor;
        int FP_ID1;
        int FP_ID2;

	/* picntrl info */
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_active;
	struct pinctrl_state  *pins_sleep;

	/* Wakelock Protect */
	struct wake_lock wake_lock;
	bool irq_wakeup_flag;

	dev_t devt;
	struct list_head device_entry;
	/* Wakelock Protect */
	/* --- common part --- */

	/* +++ SYNA +++ */
	struct device sndev;
	struct cdev cdev;
	spinlock_t vfs_spi_lock;
	struct mutex buffer_mutex;
	unsigned int is_opened;
	unsigned char *buffer;
	unsigned char *null_buffer;
	unsigned char *stream_buffer;
	size_t stream_buffer_size;
#if DO_CHIP_SELECT
	unsigned int cs_pin;
#endif
	int user_pid;
	int signal_id;
	unsigned int current_spi_speed;
	unsigned int is_drdy_irq_enabled;
	unsigned int drdy_ntf_type;
	struct mutex kernel_lock;
	/* --- SYNA --- */

	/* +++ Goodix +++ */
#if defined(USE_SPI_BUS)
		struct spi_device *spi;
#elif defined(USE_PLATFORM_BUS)
		struct platform_device *spi;
#endif
	struct clk *core_clk;
	struct clk *iface_clk;
	struct input_dev *input;
	/* buffer is NULL unless this device is open (users > 0) */
	unsigned users;
	signed irq_gpio;
	signed reset_gpio;
	signed pwr_gpio;
	int irq;
	int irq_enabled;
	int clk_enabled;
#ifdef GF_FASYNC
	struct fasync_struct *async;
#endif
	struct notifier_block notifier;
	char device_available;
	char fb_black;
	bool enable_touch_mask;
	unsigned long FpTimer_expires;
    struct timer_list FpMaskTouch_Timer;
	/* --- Goodix --- */
};
/* +++ Synaptics part +++ */
#define VALIDITY_PART_NAME "validity_fingerprint"
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_mutex);
static struct class *vfsspi_device_class;
static int gpio_irq;
#ifdef VFSSPI_32BIT
/*
 * Used by IOCTL compat command:
 *         VFSSPI_IOCTL_RW_SPI_MESSAGE
 *
 * @rx_buffer:pointer to retrieved data
 * @tx_buffer:pointer to transmitted data
 * @len:transmitted/retrieved data size
 */
struct vfsspi_compat_ioctl_transfer {
	compat_uptr_t rx_buffer;
	compat_uptr_t tx_buffer;
	unsigned int len;
};
#endif
static int vfsspi_sendDrdyEventFd(struct fp_device_data *vfsSpiDev);
static int vfsspi_sendDrdyNotify(struct fp_device_data *vfsSpiDev);
/* --- Synaptics part --- */

/* +++ goodix part +++ */
#define GF_SPIDEV_NAME      "goodix,fingerprint"
/*device name after register in charater*/
#define GF_DEV_NAME         "goodix_fp"
#define	GF_INPUT_NAME	    "goodixfp"	/*"goodix_fp" */

#define	CHRD_DRIVER_NAME	"goodix_fp_spi"
#define	CLASS_NAME		    "goodix_fp"
#define SPIDEV_MAJOR		154	/* assigned */
#define N_SPI_MINORS		32	/* ... up to 256 */

// BEGIN: jacob_kung@asus.com: keycode for fingerprint gestures
#define FRINGERPRINT_SWIPE_UP 827 // 827
#define FRINGERPRINT_SWIPE_DOWN 828 // 828
#define FRINGERPRINT_SWIPE_LEFT 829 // 829
#define FRINGERPRINT_SWIPE_RIGHT 830 // 830
#define FRINGERPRINT_TAP 831 // 831
#define FRINGERPRINT_DTAP 832 // 832
#define FRINGERPRINT_LONGPRESS 833 // 833
// END: jacob_kung@asus.com

static struct class *gf_class;
struct gf_key_map key_map[] =
{
      {  "POWER",  KEY_POWER,  KEY_POWER  },
      {  "HOME" ,  KEY_HOME,  KEY_HOME   },
      {  "MENU" ,  KEY_MENU,  KEY_MENU   },
      {  "BACK" ,  KEY_BACK,  KEY_BACK   },
      {  "FORCE",  KEY_F9,  KEY_F9     },
      {  "CLICK",  KEY_F19,  KEY_F19    },
      {  "UP"   ,  FRINGERPRINT_SWIPE_UP,  KEY_F17     },
      {  "DOWN" ,  FRINGERPRINT_SWIPE_DOWN,  KEY_F18   },
      {  "LEFT" ,  FRINGERPRINT_SWIPE_LEFT,  KEY_F19   },
      {  "RIGHT",  FRINGERPRINT_SWIPE_RIGHT,  KEY_F20  },
      {  "TAP",  FRINGERPRINT_TAP,  KEY_F21  },
      {  "DTAP",  FRINGERPRINT_DTAP,  KEY_F22  },
      {  "LONGPRESS",  FRINGERPRINT_LONGPRESS,  KEY_F23  },

};

static void init_FpMaskTouch_Timer(struct fp_device_data *gf_dev);
static void del_FpMaskTouch_Timer(struct fp_device_data *gf_dev);

/**************************debug******************************/
#define GF_DEBUG
/*#undef  GF_DEBUG*/

#ifdef  GF_DEBUG
#define gf_dbg(fmt, args...) do { \
					pr_warn("gf:" fmt, ##args);\
		} while (0)
#define FUNC_ENTRY()  pr_warn("gf:%s, entry\n", __func__)
#define FUNC_EXIT()  pr_warn("gf:%s, exit\n", __func__)
#else
#define gf_dbg(fmt, args...)
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

/*Global variables*/
/*static MODE g_mode = GF_IMAGE_MODE;*/
//static DECLARE_BITMAP(minors, N_SPI_MINORS);
//static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
//static struct fp_device_data gf;

/* --- goodix part --- */


/* +++ Synaptics operate zone +++ */
#define ELAN_CALI_TIMEOUT_MSEC	10000
struct completion cmd_done;

static int vfsspi_send_drdy_signal(struct fp_device_data *vfsspi_device)
{
	struct task_struct *t;
	int ret = 0;

	pr_debug("vfsspi_send_drdy_signal\n");

	if (vfsspi_device->user_pid != 0) {
		rcu_read_lock();
		/* find the task_struct associated with userpid */
		pr_debug("Searching task with PID=%08x\n",
			vfsspi_device->user_pid);
		t = pid_task(find_pid_ns(vfsspi_device->user_pid, &init_pid_ns),
			     PIDTYPE_PID);
		if (t == NULL) {
			pr_debug("No such pid\n");
			rcu_read_unlock();
			return -ENODEV;
		}
		rcu_read_unlock();
		/* notify DRDY signal to user process */
		ret = send_sig_info(vfsspi_device->signal_id,
				    (struct siginfo *)1, t);
		if (ret < 0)
			pr_err("Error sending signal\n");

	} else {
		pr_err("pid not received yet\n");
	}

	return ret;
}
#if 0
/* Return no. of bytes written to device. Negative number for errors */
static inline ssize_t vfsspi_writeSync(struct fp_device_data *vfsspi_device,
					size_t len)
{
	int    status = 0;
	struct spi_message m;
	struct spi_transfer t;

	pr_debug("vfsspi_writeSync\n");

	spi_message_init(&m);
	memset(&t, 0, sizeof(t));

	t.rx_buf = vfsspi_device->null_buffer;
	t.tx_buf = vfsspi_device->buffer;
	t.len = len;
	t.speed_hz = vfsspi_device->current_spi_speed;

	spi_message_add_tail(&t, &m);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 0);
#endif
	status = spi_sync(vfsspi_device->spi, &m);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 1);
#endif
	if (status == 0)
		status = m.actual_length;
	pr_debug("vfsspi_writeSync,length=%d\n", status);
	return status;
}

/* Return no. of bytes read > 0. negative integer incase of error. */
static inline ssize_t vfsspi_readSync(struct fp_device_data *vfsspi_device,
					size_t len)
{
	int    status = 0;
	struct spi_message m;
	struct spi_transfer t;

	pr_debug("vfsspi_readSync\n");

	spi_message_init(&m);
	memset(&t, 0x0, sizeof(t));

	memset(vfsspi_device->null_buffer, 0x0, len);
	t.tx_buf = vfsspi_device->null_buffer;
	t.rx_buf = vfsspi_device->buffer;
	t.len = len;
	t.speed_hz = vfsspi_device->current_spi_speed;

	spi_message_add_tail(&t, &m);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 0);
#endif
	status = spi_sync(vfsspi_device->spi, &m);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 1);
#endif
	if (status == 0)
		status = len;

	pr_debug("vfsspi_readSync,length=%d\n", status);

	return status;
}

static ssize_t vfsspi_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *fPos)
{
	struct fp_device_data *vfsspi_device = NULL;
	ssize_t               status = 0;

	pr_debug("vfsspi_write\n");

	if (count > DEFAULT_BUFFER_SIZE || count <= 0)
		return -EMSGSIZE;

	vfsspi_device = filp->private_data;

	mutex_lock(&vfsspi_device->buffer_mutex);

	if (vfsspi_device->buffer) {
		unsigned long missing = 0;

		missing = copy_from_user(vfsspi_device->buffer, buf, count);

		if (missing == 0)
			status = vfsspi_writeSync(vfsspi_device, count);
		else
			status = -EFAULT;
	}

	mutex_unlock(&vfsspi_device->buffer_mutex);

	return status;
}

static ssize_t vfsspi_read(struct file *filp, char __user *buf,
			size_t count, loff_t *fPos)
{
	struct fp_device_data *vfsspi_device = NULL;
	ssize_t                status    = 0;

	pr_debug("vfsspi_read\n");

	if (count > DEFAULT_BUFFER_SIZE || count <= 0)
		return -EMSGSIZE;
	if (buf == NULL)
		return -EFAULT;


	vfsspi_device = filp->private_data;

	mutex_lock(&vfsspi_device->buffer_mutex);

	status  = vfsspi_readSync(vfsspi_device, count);


	if (status > 0) {
		unsigned long missing = 0;
		/* data read. Copy to user buffer.*/
		missing = copy_to_user(buf, vfsspi_device->buffer, status);

		if (missing == status) {
			pr_err("copy_to_user failed\n");
			/* Nothing was copied to user space buffer. */
			status = -EFAULT;
		} else {
			status = status - missing;
		}
	}

	mutex_unlock(&vfsspi_device->buffer_mutex);

	return status;
}

static int vfsspi_xfer(struct fp_device_data *vfsspi_device,
			struct vfsspi_ioctl_transfer *tr)
{
	int status = 0;
	struct spi_message m;
	struct spi_transfer t;

	pr_debug("vfsspi_xfer\n");

	if (vfsspi_device == NULL || tr == NULL)
		return -EFAULT;

	if (tr->len > DEFAULT_BUFFER_SIZE || tr->len <= 0)
		return -EMSGSIZE;

	if (tr->tx_buffer != NULL) {

		if (copy_from_user(vfsspi_device->null_buffer,
				tr->tx_buffer, tr->len) != 0)
			return -EFAULT;
	}

	spi_message_init(&m);
	memset(&t, 0, sizeof(t));

	t.tx_buf = vfsspi_device->null_buffer;
	t.rx_buf = vfsspi_device->buffer;
	t.len = tr->len;
	t.speed_hz = vfsspi_device->current_spi_speed;

	spi_message_add_tail(&t, &m);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 0);
#endif
	status = spi_sync(vfsspi_device->spi, &m);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 1);
#endif
	if (status == 0) {
		if (tr->rx_buffer != NULL) {
			unsigned missing = 0;

			missing = copy_to_user(tr->rx_buffer,
					       vfsspi_device->buffer, tr->len);

			if (missing != 0)
				tr->len = tr->len - missing;
		}
	}
	pr_debug("vfsspi_xfer,length=%d\n", tr->len);
	return status;

} /* vfsspi_xfer */

static int vfsspi_rw_spi_message(struct fp_device_data *vfsspi_device,
				 unsigned long arg)
{
	struct vfsspi_ioctl_transfer   *dup  = NULL;
#ifdef VFSSPI_32BIT
    struct vfsspi_compat_ioctl_transfer   dup_compat;
#endif	
	dup = kmalloc(sizeof(struct vfsspi_ioctl_transfer), GFP_KERNEL);
	if (dup == NULL)
		return -ENOMEM;
#ifdef VFSSPI_32BIT
	if (copy_from_user(&dup_compat, (void __user *)arg,
			   sizeof(struct vfsspi_compat_ioctl_transfer)) != 0)  {
#else
	if (copy_from_user(dup, (void __user *)arg,
			   sizeof(struct vfsspi_ioctl_transfer)) != 0)  {
#endif
		return -EFAULT;
	} else {
		int err;
#ifdef VFSSPI_32BIT		
		dup->rx_buffer = (unsigned char *)(unsigned long)dup_compat.rx_buffer;
		dup->tx_buffer = (unsigned char *)(unsigned long)dup_compat.tx_buffer;
		dup->len = dup_compat.len;
#endif
		err = vfsspi_xfer(vfsspi_device, dup);
		if (err != 0) {
			kfree(dup);
			return err;
		}
	}
#ifdef VFSSPI_32BIT
    dup_compat.len = dup->len;
	if (copy_to_user((void __user *)arg, &dup_compat,
			 sizeof(struct vfsspi_compat_ioctl_transfer)) != 0){
#else
	if (copy_to_user((void __user *)arg, dup,
			 sizeof(struct vfsspi_ioctl_transfer)) != 0){
#endif
		kfree(dup);
		return -EFAULT;
	}
	kfree(dup);
	return 0;
}

static int vfsspi_set_clk(struct fp_device_data *vfsspi_device,
			  unsigned long arg)
{
	unsigned short clock = 0;
	struct spi_device *spidev = NULL;

	if (copy_from_user(&clock, (void __user *)arg,
			   sizeof(unsigned short)) != 0)
		return -EFAULT;

	spin_lock_irq(&vfsspi_device->vfs_spi_lock);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 0);
#endif
	spidev = spi_dev_get(vfsspi_device->spi);
#if DO_CHIP_SELECT
	gpio_set_value(vfsspi_device->cs_pin, 1);
#endif
	spin_unlock_irq(&vfsspi_device->vfs_spi_lock);
	if (spidev != NULL) {
		switch (clock) {
		case 0:	/* Running baud rate. */
			pr_debug("Running baud rate.\n");
			spidev->max_speed_hz = MAX_BAUD_RATE;
			vfsspi_device->current_spi_speed = MAX_BAUD_RATE;
			break;
		case 0xFFFF: /* Slow baud rate */
			pr_debug("slow baud rate.\n");
			spidev->max_speed_hz = SLOW_BAUD_RATE;
			vfsspi_device->current_spi_speed = SLOW_BAUD_RATE;
			break;
		default:
			pr_debug("baud rate is %d.\n", clock);
			vfsspi_device->current_spi_speed =
				clock * BAUD_RATE_COEF;
			if (vfsspi_device->current_spi_speed > MAX_BAUD_RATE)
				vfsspi_device->current_spi_speed =
					MAX_BAUD_RATE;
			spidev->max_speed_hz = vfsspi_device->current_spi_speed;
			break;
		}
		spi_dev_put(spidev);
	}
	return 0;
}
#endif
static int vfsspi_register_drdy_signal(struct fp_device_data *vfsspi_device,
				       unsigned long arg)
{
	struct vfsspi_ioctl_register_signal usr_signal;
	if (copy_from_user(&usr_signal, (void __user *)arg, sizeof(usr_signal)) != 0) {
		pr_err("Failed copy from user.\n");
		return -EFAULT;
	} else {
		vfsspi_device->user_pid = usr_signal.user_pid;
		vfsspi_device->signal_id = usr_signal.signal_id;
	}
	return 0;
}

static irqreturn_t vfsspi_irq(int irq, void *context)
{
	struct fp_device_data *vfsspi_device = context;
	/* Linux kernel is designed so that when you disable
	an edge-triggered interrupt, and the edge happens while
	the interrupt is disabled, the system will re-play the
	interrupt at enable time.
	Therefore, we are checking DRDY GPIO pin state to make sure
	if the interrupt handler has been called actually by DRDY
	interrupt and it's not a previous interrupt re-play */
	if (gpio_get_value(vfsspi_device->drdy_pin) == DRDY_ACTIVE_STATUS) {
		wake_lock_timeout(&vfsspi_device->wake_lock, 1000);
		vfsspi_sendDrdyNotify(vfsspi_device);
	}

	return IRQ_HANDLED;
}

static int vfsspi_sendDrdyEventFd(struct fp_device_data *vfsSpiDev)
{
    struct task_struct *t;
    struct file *efd_file = NULL;
    struct eventfd_ctx *efd_ctx = NULL;	int ret = 0;

    pr_debug("vfsspi_sendDrdyEventFd\n");

    if (vfsSpiDev->user_pid != 0) {
        rcu_read_lock();
        /* find the task_struct associated with userpid */
        pr_debug("Searching task with PID=%08x\n", vfsSpiDev->user_pid);
        t = pid_task(find_pid_ns(vfsSpiDev->user_pid, &init_pid_ns),
            PIDTYPE_PID);
        if (t == NULL) {
            pr_debug("No such pid\n");
            rcu_read_unlock();
            return -ENODEV;
        }
        efd_file = fcheck_files(t->files, vfsSpiDev->signal_id);
        rcu_read_unlock();

        if (efd_file == NULL) {
            pr_debug("No such efd_file\n");
            return -ENODEV;
        }
        
        efd_ctx = eventfd_ctx_fileget(efd_file);
        if (efd_ctx == NULL) {
            pr_debug("eventfd_ctx_fileget is failed\n");
            return -ENODEV;
        }

        /* notify DRDY eventfd to user process */
        eventfd_signal(efd_ctx, 1);

        /* Release eventfd context */
        eventfd_ctx_put(efd_ctx);
    }

    return ret;
}

static int vfsspi_sendDrdyNotify(struct fp_device_data *vfsSpiDev)
{
    int ret = 0;

    if (vfsSpiDev->drdy_ntf_type == VFSSPI_DRDY_NOTIFY_TYPE_EVENTFD) {
        ret = vfsspi_sendDrdyEventFd(vfsSpiDev);
    } else {
        ret = vfsspi_send_drdy_signal(vfsSpiDev);
    }

    return ret;
}

static int vfsspi_enableIrq(struct fp_device_data *vfsspi_device)
{
	pr_debug("vfsspi_enableIrq\n");

	if (vfsspi_device->is_drdy_irq_enabled == DRDY_IRQ_ENABLE) {
		pr_debug("DRDY irq already enabled\n");
		return -EINVAL;
	}

	enable_irq(gpio_irq);
	vfsspi_device->is_drdy_irq_enabled = DRDY_IRQ_ENABLE;

	return 0;
}

static int vfsspi_disableIrq(struct fp_device_data *vfsspi_device)
{
	pr_debug("vfsspi_disableIrq\n");

	if (vfsspi_device->is_drdy_irq_enabled == DRDY_IRQ_DISABLE) {
		pr_debug("DRDY irq already disabled\n");
		return -EINVAL;
	}

	disable_irq_nosync(gpio_irq);
	vfsspi_device->is_drdy_irq_enabled = DRDY_IRQ_DISABLE;

	return 0;
}
static int vfsspi_set_drdy_int(struct fp_device_data *vfsspi_device,
			       unsigned long arg)
{
	unsigned short drdy_enable_flag;
	if (copy_from_user(&drdy_enable_flag, (void __user *)arg,
			   sizeof(drdy_enable_flag)) != 0) {
		pr_err("Failed copy from user.\n");
		return -EFAULT;
	}
	if (drdy_enable_flag == 0)
			vfsspi_disableIrq(vfsspi_device);
	else {
			vfsspi_enableIrq(vfsspi_device);
			/* Workaround the issue where the system
			  misses DRDY notification to host when
			  DRDY pin was asserted before enabling
			  device.*/
			if (gpio_get_value(vfsspi_device->drdy_pin) ==
				DRDY_ACTIVE_STATUS) {
				vfsspi_sendDrdyNotify(vfsspi_device);
			}
	}
	return 0;
}


static void vfsspi_hardReset(struct fp_device_data *vfsspi_device)
{
	pr_debug("vfsspi_hardReset\n");
	printk("[Jacob] vfsspi sleep pin = %d \n", vfsspi_device->sleep_pin);
	if (vfsspi_device != NULL) {
		gpio_set_value(vfsspi_device->sleep_pin, 0);
		mdelay(1);
		gpio_set_value(vfsspi_device->sleep_pin, 1);
		mdelay(5);
	}
}


static void vfsspi_suspend(struct fp_device_data *vfsspi_device)
{
	pr_debug("vfsspi_suspend\n");
	printk("[Jacob] vfsspi sleep pin suspend\n");

	if (vfsspi_device != NULL) {
		spin_lock(&vfsspi_device->vfs_spi_lock);
		gpio_set_value(vfsspi_device->sleep_pin, 0);
		spin_unlock(&vfsspi_device->vfs_spi_lock);
	}
}

static long vfsspi_ioctl(struct file *filp, unsigned int cmd,
			unsigned long arg)
{
	int ret_val = 0;
	struct fp_device_data *vfsspi_device = NULL;

	pr_debug("vfsspi_ioctl\n");

	if (_IOC_TYPE(cmd) != VFSSPI_IOCTL_MAGIC) {
		pr_err("invalid magic. cmd=0x%X Received=0x%X Expected=0x%X\n",
			cmd, _IOC_TYPE(cmd), VFSSPI_IOCTL_MAGIC);
		return -ENOTTY;
	}

	vfsspi_device = filp->private_data;
	
	mutex_lock(&vfsspi_device->buffer_mutex);
	switch (cmd) {
	case VFSSPI_IOCTL_DEVICE_RESET:
		pr_debug("VFSSPI_IOCTL_DEVICE_RESET:\n");
		vfsspi_hardReset(vfsspi_device);
		break;
	case VFSSPI_IOCTL_DEVICE_SUSPEND:
	{
		pr_debug("VFSSPI_IOCTL_DEVICE_SUSPEND:\n");
		vfsspi_suspend(vfsspi_device);
		break;
	}		
#if 0
	case VFSSPI_IOCTL_RW_SPI_MESSAGE:
		pr_debug("VFSSPI_IOCTL_RW_SPI_MESSAGE");
		ret_val = vfsspi_rw_spi_message(vfsspi_device, arg);
		break;
	case VFSSPI_IOCTL_SET_CLK:
		pr_debug("VFSSPI_IOCTL_SET_CLK\n");
		ret_val = vfsspi_set_clk(vfsspi_device, arg);
		break;
#endif
	case VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL:
		pr_debug("VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL\n");
		ret_val = vfsspi_register_drdy_signal(vfsspi_device, arg);
		break;
	case VFSSPI_IOCTL_SET_DRDY_INT:
		pr_debug("VFSSPI_IOCTL_SET_DRDY_INT\n");
		ret_val = vfsspi_set_drdy_int(vfsspi_device, arg);
		break;
	case VFSSPI_IOCTL_SELECT_DRDY_NTF_TYPE:
        {
            vfsspi_iocSelectDrdyNtfType_t drdyTypes;

            pr_debug("VFSSPI_IOCTL_SELECT_DRDY_NTF_TYPE\n");

            if (copy_from_user(&drdyTypes, (void __user *)arg,
                sizeof(vfsspi_iocSelectDrdyNtfType_t)) != 0) {
                    pr_debug("copy from user failed.\n");
                    ret_val = -EFAULT;
            } else {
                if (0 != (drdyTypes.supportedTypes & VFSSPI_DRDY_NOTIFY_TYPE_EVENTFD)) {
                    vfsspi_device->drdy_ntf_type = VFSSPI_DRDY_NOTIFY_TYPE_EVENTFD;
                } else {
                    vfsspi_device->drdy_ntf_type = VFSSPI_DRDY_NOTIFY_TYPE_SIGNAL;
                }
                drdyTypes.selectedType = vfsspi_device->drdy_ntf_type;
                if (copy_to_user((void __user *)arg, &(drdyTypes),
                    sizeof(vfsspi_iocSelectDrdyNtfType_t)) == 0) {
                        ret_val = 0;
                } else {
                    pr_debug("copy to user failed\n");
                }
            }
            break;
        }
	default:
		ret_val = -EFAULT;
		break;
	}
	mutex_unlock(&vfsspi_device->buffer_mutex);
	return ret_val;
}

static int vfsspi_open(struct inode *inode, struct file *filp)
{
	struct fp_device_data *vfsspi_device = NULL;
	int status = -ENXIO;

	pr_debug("vfsspi_open\n");

	mutex_lock(&device_list_mutex);

	list_for_each_entry(vfsspi_device, &device_list, device_entry) {
		if (vfsspi_device->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}

	if (status == 0) {
		mutex_lock(&vfsspi_device->kernel_lock);
		if (vfsspi_device->is_opened != 0) {
			status = -EBUSY;
			pr_err("vfsspi_open: is_opened != 0, -EBUSY");
			goto vfsspi_open_out;
		}
		vfsspi_device->user_pid = 0;
        vfsspi_device->drdy_ntf_type = VFSSPI_DRDY_NOTIFY_TYPE_SIGNAL;
		if (vfsspi_device->buffer != NULL) {
			pr_err("vfsspi_open: buffer != NULL");
			goto vfsspi_open_out;
		}
		vfsspi_device->null_buffer =
			kmalloc(DEFAULT_BUFFER_SIZE, GFP_KERNEL);
		if (vfsspi_device->null_buffer == NULL) {
			status = -ENOMEM;
			pr_err("vfsspi_open: null_buffer == NULL, -ENOMEM");
			goto vfsspi_open_out;
		}
		vfsspi_device->buffer =
			kmalloc(DEFAULT_BUFFER_SIZE, GFP_KERNEL);
		if (vfsspi_device->buffer == NULL) {
			status = -ENOMEM;
			kfree(vfsspi_device->null_buffer);
			pr_err("vfsspi_open: buffer == NULL, -ENOMEM");
			goto vfsspi_open_out;
		}
		vfsspi_device->is_opened = 1;
		filp->private_data = vfsspi_device;
		nonseekable_open(inode, filp);

vfsspi_open_out:
		mutex_unlock(&vfsspi_device->kernel_lock);
	}
	mutex_unlock(&device_list_mutex);
	return status;
}


static int vfsspi_release(struct inode *inode, struct file *filp)
{
	struct fp_device_data *vfsspi_device = NULL;
	int                   status     = 0;

	pr_debug("vfsspi_release\n");

	mutex_lock(&device_list_mutex);
	vfsspi_device = filp->private_data;
	filp->private_data = NULL;
	vfsspi_device->is_opened = 0;
	if (vfsspi_device->buffer != NULL) {
		kfree(vfsspi_device->buffer);
		vfsspi_device->buffer = NULL;
	}

	if (vfsspi_device->null_buffer != NULL) {
		kfree(vfsspi_device->null_buffer);
		vfsspi_device->null_buffer = NULL;
	}

	mutex_unlock(&device_list_mutex);
	return status;
}

/* file operations associated with device */
static const struct file_operations vfsspi_fops = {
	.owner   = THIS_MODULE,
//	.write   = vfsspi_write,
//	.read    = vfsspi_read,
	.unlocked_ioctl   = vfsspi_ioctl,
	.open    = vfsspi_open,
	.release = vfsspi_release,
};

/* --- Synaptics operate zone --- */

/* +++ goodix operate zone +++ */
void gf_cleanup(struct fp_device_data	* gf_dev)
{
    pr_info("[info] %s\n",__func__);
    if (gpio_is_valid(gf_dev->irq_gpio)){
        gpio_free(gf_dev->irq_gpio);
        pr_info("remove irq_gpio success\n");
    }
    if (gpio_is_valid(gf_dev->reset_gpio)){
        gpio_free(gf_dev->reset_gpio);
        pr_info("remove reset_gpio success\n");
    }
	if (gpio_is_valid(gf_dev->pwr_gpio)){
        gpio_free(gf_dev->pwr_gpio);
        pr_info("remove pwr_gpio success\n");
    }

}

/*power management*/
int gf_power_on(struct fp_device_data* gf_dev)
{
    int rc = 0;
	if (gpio_is_valid(gf_dev->pwr_gpio)) {
        gpio_set_value(gf_dev->pwr_gpio, 1);
    }

    msleep(10);
    pr_info("---- power on ok ----\n");

    return rc;
}

int gf_power_off(struct fp_device_data* gf_dev)
{
    int rc = 0;			
	if (gpio_is_valid(gf_dev->pwr_gpio)) {
        gpio_set_value(gf_dev->pwr_gpio, 1);
    }

    pr_info("---- power off ----\n");
    return rc;
}

/********************************************************************
 *CPU output low level in RST pin to reset GF. This is the MUST action for GF.
 *Take care of this function. IO Pin driver strength / glitch and so on.
 ********************************************************************/
int gf_hw_reset(struct fp_device_data *gf_dev, unsigned int delay_ms)
{	
    if(gf_dev == NULL) {
        pr_info("Input Device is NULL.\n");
        return -1;
    }
    gpio_direction_output(gf_dev->reset_gpio, 1);
    gpio_set_value(gf_dev->reset_gpio, 0);
    mdelay(3);
    gpio_set_value(gf_dev->reset_gpio, 1);
    mdelay(delay_ms);
    return 0;
}

int gf_irq_num(struct fp_device_data *gf_dev)
{
    if(gf_dev == NULL) {
        pr_info("Input Device is NULL.\n");
        return -1;
    } else {
        return gpio_to_irq(gf_dev->irq_gpio);
    }
}

static void gf_enable_irq(struct fp_device_data *gf_dev)
{
	if (gf_dev->irq_enabled) {
		pr_warn("IRQ has been enabled.\n");
	} else {
		enable_irq(gf_dev->irq);
		gf_dev->irq_enabled = 1;
	}
}

static void gf_disable_irq(struct fp_device_data *gf_dev)
{
	if (gf_dev->irq_enabled) {
		gf_dev->irq_enabled = 0;
		disable_irq(gf_dev->irq);
	} else {
		pr_warn("IRQ has been disabled.\n");
	}
}

#ifdef AP_CONTROL_CLK
static long spi_clk_max_rate(struct clk *clk, unsigned long rate)
{
	long lowest_available, nearest_low, step_size, cur;
	long step_direction = -1;
	long guess = rate;
	int max_steps = 10;

	cur = clk_round_rate(clk, rate);
	if (cur == rate)
		return rate;

	/* if we got here then: cur > rate */
	lowest_available = clk_round_rate(clk, 0);
	if (lowest_available > rate)
		return -EINVAL;

	step_size = (rate - lowest_available) >> 1;
	nearest_low = lowest_available;

	while (max_steps-- && step_size) {
		guess += step_size * step_direction;
		cur = clk_round_rate(clk, guess);

		if ((cur < rate) && (cur > nearest_low))
			nearest_low = cur;
		/*
		 * if we stepped too far, then start stepping in the other
		 * direction with half the step size
		 */
		if (((cur > rate) && (step_direction > 0))
		    || ((cur < rate) && (step_direction < 0))) {
			step_direction = -step_direction;
			step_size >>= 1;
		}
	}
	return nearest_low;
}

static void spi_clock_set(struct fp_device_data *gf_dev, int speed)
{
	long rate;
	int rc;

	rate = spi_clk_max_rate(gf_dev->core_clk, speed);
	if (rate < 0) {
		pr_info("%s: no match found for requested clock frequency:%d",
			__func__, speed);
		return;
	}

	rc = clk_set_rate(gf_dev->core_clk, rate);
}

static int gfspi_ioctl_clk_init(struct fp_device_data *data)
{
	pr_debug("%s: enter\n", __func__);

	data->clk_enabled = 0;
	data->core_clk = clk_get(&data->spi->dev, "core_clk");
	if (IS_ERR_OR_NULL(data->core_clk)) {
		pr_err("%s: fail to get core_clk\n", __func__);
		return -1;
	}
	data->iface_clk = clk_get(&data->spi->dev, "iface_clk");
	if (IS_ERR_OR_NULL(data->iface_clk)) {
		pr_err("%s: fail to get iface_clk\n", __func__);
		clk_put(data->core_clk);
		data->core_clk = NULL;
		return -2;
	}
	return 0;
}

static int gfspi_ioctl_clk_enable(struct fp_device_data *data)
{
	int err;

	pr_debug("%s: enter\n", __func__);

	if (data->clk_enabled)
		return 0;

	err = clk_prepare_enable(data->core_clk);
	if (err) {
		pr_err("%s: fail to enable core_clk\n", __func__);
		return -1;
	}

	err = clk_prepare_enable(data->iface_clk);
	if (err) {
		pr_err("%s: fail to enable iface_clk\n", __func__);
		clk_disable_unprepare(data->core_clk);
		return -2;
	}

	data->clk_enabled = 1;

	return 0;
}

static int gfspi_ioctl_clk_disable(struct fp_device_data *data)
{
	pr_debug("%s: enter\n", __func__);

	if (!data->clk_enabled)
		return 0;

	clk_disable_unprepare(data->core_clk);
	clk_disable_unprepare(data->iface_clk);
	data->clk_enabled = 0;

	return 0;
}

static int gfspi_ioctl_clk_uninit(struct fp_device_data *data)
{
	pr_debug("%s: enter\n", __func__);

	if (data->clk_enabled)
		gfspi_ioctl_clk_disable(data);

	if (!IS_ERR_OR_NULL(data->core_clk)) {
		clk_put(data->core_clk);
		data->core_clk = NULL;
	}

	if (!IS_ERR_OR_NULL(data->iface_clk)) {
		clk_put(data->iface_clk);
		data->iface_clk = NULL;
	}

	return 0;
}
#endif

static long gf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct fp_device_data *gf_dev = filp->private_data;
	struct gf_key gf_key = { 0 };
	int retval = 0;
        int i;
#ifdef AP_CONTROL_CLK
	unsigned int speed = 0;
#endif
	FUNC_ENTRY();
	if (_IOC_TYPE(cmd) != GF_IOC_MAGIC)
		return -ENODEV;

	if (_IOC_DIR(cmd) & _IOC_READ)
		retval =
		    !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	if ((retval == 0) && (_IOC_DIR(cmd) & _IOC_WRITE))
		retval =
		    !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (retval)
		return -EFAULT;
    
	if(gf_dev->device_available == 0) {
        if((cmd == GF_IOC_POWER_ON) || (cmd == GF_IOC_POWER_OFF)){
            pr_info("power cmd\n");
        }
        else{
            pr_info("Sensor is power off currently. \n");
            return -ENODEV;
        }
    }

    printk("[gf_ioctl][Jacob] cmd = %d \n", cmd);

	switch (cmd) {
	case GF_IOC_DISABLE_IRQ:
		gf_disable_irq(gf_dev);
		break;
	case GF_IOC_ENABLE_IRQ:
		gf_enable_irq(gf_dev);
		break;
	case GF_IOC_SETSPEED:
#ifdef AP_CONTROL_CLK
   		retval = __get_user(speed, (u32 __user *) arg);
    	if (retval == 0) {
	    	if (speed > 8 * 1000 * 1000) {
		    	pr_warn("Set speed:%d is larger than 8Mbps.\n",	speed);
		    } else {
			    spi_clock_set(gf_dev, speed);
		    }
	    } else {
		   pr_warn("Failed to get speed from user. retval = %d\n",	retval);
		}
#else
        pr_info("This kernel doesn't support control clk in AP\n");
#endif
		break;
	case GF_IOC_RESET:
		gf_hw_reset(gf_dev, 70);
		break;
	case GF_IOC_COOLBOOT:
		gf_power_off(gf_dev);
		mdelay(5);
		gf_power_on(gf_dev);
		break;
	case GF_IOC_SENDKEY:
		if (copy_from_user
		    (&gf_key, (struct gf_key *)arg, sizeof(struct gf_key))) {
			pr_warn("Failed to copy data from user space.\n");
			retval = -EFAULT;
			break;
		}

		for(i = 0; i< ARRAY_SIZE(key_map); i++) {
			if(key_map[i].val == gf_key.key){
				input_report_key(gf_dev->input, key_map[i].report_val, gf_key.value);
				input_sync(gf_dev->input);
				break;
			}
		}

		if(i == ARRAY_SIZE(key_map)) {
			pr_warn("key %d not support yet \n", gf_key.key);
			retval = -EFAULT;
		}

		break;
	case GF_IOC_CLK_READY:
#ifdef AP_CONTROL_CLK
        gfspi_ioctl_clk_enable(gf_dev);
#else
        pr_info("Doesn't support control clock.\n");
#endif
		break;
	case GF_IOC_CLK_UNREADY:
#ifdef AP_CONTROL_CLK
        gfspi_ioctl_clk_disable(gf_dev);
#else
        pr_info("Doesn't support control clock.\n");
#endif
		break;
	case GF_IOC_PM_FBCABCK:
		__put_user(gf_dev->fb_black, (u8 __user *) arg);
		break;
    case GF_IOC_POWER_ON:
        if(gf_dev->device_available == 1)
            pr_info("Sensor has already powered-on.\n");
        else
            gf_power_on(gf_dev);
        gf_dev->device_available = 1;
        break;
    case GF_IOC_POWER_OFF:
        if(gf_dev->device_available == 0)
            pr_info("Sensor has already powered-off.\n");
        else
            gf_power_off(gf_dev);
        gf_dev->device_available = 0;
        break;
    case GF_IOC_TOUCH_ENABLE_MASK:
		gf_dev->enable_touch_mask = 1;
        pr_info("[JK] GF_IOC_TOUCH_ENABLE_MASK !\n");
        break;
    case GF_IOC_TOUCH_DISABLE_MASK:
		del_FpMaskTouch_Timer(gf_dev);
		gf_dev->enable_touch_mask = 0;
		g_FP_Disable_Touch = false;
        pr_info("[JK] GF_IOC_TOUCH_DISABLE_MASK !\n");
        break;
    case GF_IOC_CLEAN_EARLY_WAKE_HINT:
	    gf_dev->FP_ID2 = 1;
        break;
	default:
		gf_dbg("Unsupport cmd:0x%x\n", cmd);
		break;
	}

	FUNC_EXIT();
	return retval;
}

#ifdef CONFIG_COMPAT
static long gf_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return gf_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif /*CONFIG_COMPAT*/

static irqreturn_t gf_irq(int irq, void *handle)
{
	struct fp_device_data *gf_dev = handle;
        char temp = GF_NET_EVENT_IRQ;

	printk("[Goodix] IRQ recive \n");

	kobject_uevent(&gf_dev->spi->dev.kobj, KOBJ_CHANGE);

#if GF_EARLY_WAKE
	if ((!g_update_bl) & gf_dev->FP_ID2) {
		input_report_key(gf_dev->input, KEY_F22, 1);
		input_sync(gf_dev->input);
		input_report_key(gf_dev->input, KEY_F22, 0);
		input_sync(gf_dev->input);
		gf_dev->FP_ID2 = 0;
	} else {
	    gf_dev->FP_ID2 = 0;
    }

#endif

	wake_lock_timeout(&gf_dev->wake_lock, msecs_to_jiffies(1000));

        if (gf_dev->module_vendor == vendor_module_gdix_3266A) {
                sendnlmsg(&temp);
        }  else {
#ifdef GF_FASYNC
        	if (gf_dev->async)
        		kill_fasync(&gf_dev->async, SIGIO, POLL_IN);
#endif
        }
	if (gf_dev->enable_touch_mask) {
		g_FP_Disable_Touch = true;
	    init_FpMaskTouch_Timer(gf_dev);
	}

	return IRQ_HANDLED;
}
EXPORT_SYMBOL(g_FP_Disable_Touch);
static int gf_open(struct inode *inode, struct file *filp)
{
	struct fp_device_data *gf_dev;
	int status = -ENXIO;

	FUNC_ENTRY();
	mutex_lock(&device_list_lock);

	list_for_each_entry(gf_dev, &device_list, device_entry) {
		if (gf_dev->devt == inode->i_rdev) {
			gf_dbg("Device Found\n");
			status = 0;
			break;
		}
	}

	if (status == 0) {
		if (status == 0) {
			gf_dev->users++;
			filp->private_data = gf_dev;
			nonseekable_open(inode, filp);
			gf_dbg("Succeed to open device. irq = %d\n", gf_dev->irq);
			if (gf_dev->users == 1)
				gf_enable_irq(gf_dev);
            /*power the sensor*/
            gf_power_on(gf_dev);
		    gf_hw_reset(gf_dev, 360);
            gf_dev->device_available = 1;
		}
	} else {
		gf_dbg("No device for minor %d\n", iminor(inode));
	}
	mutex_unlock(&device_list_lock);
	FUNC_EXIT();
	return status;
}

#ifdef GF_FASYNC
static int gf_fasync(int fd, struct file *filp, int mode)
{
	struct fp_device_data *gf_dev = filp->private_data;
	int ret;

	FUNC_ENTRY();
	ret = fasync_helper(fd, filp, mode, &gf_dev->async);
	FUNC_EXIT();
	gf_dbg("ret = %d\n", ret);
	return ret;
}
#endif

static int gf_release(struct inode *inode, struct file *filp)
{
	struct fp_device_data *gf_dev;
	int status = 0;

	FUNC_ENTRY();
	mutex_lock(&device_list_lock);
	gf_dev = filp->private_data;
	filp->private_data = NULL;

	/*last close?? */
	gf_dev->users--;
	if (!gf_dev->users) {
		gf_dbg("disble_irq. irq = %d\n", gf_dev->irq);
		gf_disable_irq(gf_dev);
        /*power off the sensor*/
        gf_dev->device_available = 0;
        gf_power_off(gf_dev);
	}
	mutex_unlock(&device_list_lock);
	FUNC_EXIT();
	return status;
}

static const struct file_operations gf_fops = {
	.owner = THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.  It'll simplify things
	 * too, except for the locking.
	*/
	.unlocked_ioctl = gf_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gf_compat_ioctl,
#endif /*CONFIG_COMPAT*/
	.open = gf_open,
	.release = gf_release,
#ifdef GF_FASYNC
	.fasync = gf_fasync,
#endif
};

static int goodix_fb_state_chg_callback(struct notifier_block *nb,
					unsigned long val, void *data)
{
	struct fp_device_data *gf_dev;
	struct fb_event *evdata = data;
	unsigned int blank;
        char temp = 0;

	if (val != FB_EARLY_EVENT_BLANK)
		return 0;
	pr_info("[info] %s go to the goodix_fb_state_chg_callback value = %d\n",
		__func__, (int)val);
	gf_dev = container_of(nb, struct fp_device_data, notifier);
	if (evdata && evdata->data && val == FB_EARLY_EVENT_BLANK && gf_dev) {
		blank = *(int *)(evdata->data);
		switch (blank) {
		case FB_BLANK_POWERDOWN:
			if (gf_dev->device_available == 1) {
				gf_dev->fb_black = 1;
//				gf_dev->FP_ID2 = 1;
				g_FP_Disable_Touch = false;
                                if (gf_dev->module_vendor == vendor_module_gdix_3266A) {
                                        temp = GF_NET_EVENT_FB_BLACK;
                                        sendnlmsg(&temp);
                                } else {
#ifdef GF_FASYNC
                                        if (gf_dev->async) {
                                        	kill_fasync(&gf_dev->async, SIGIO, POLL_IN);
                                        }
#endif
                                }
				/*device unavailable */
				//gf_dev->device_available = 0;
			}
			break;
		case FB_BLANK_UNBLANK:
			if (gf_dev->device_available == 1) {
				gf_dev->fb_black = 0;
//				gf_dev->FP_ID2 = 0;
                                if (gf_dev->module_vendor == vendor_module_gdix_3266A) {
                                        temp = GF_NET_EVENT_FB_UNBLACK;
                                        sendnlmsg(&temp);
                                } else {
#ifdef GF_FASYNC
                                        if (gf_dev->async) {
                                        	kill_fasync(&gf_dev->async, SIGIO, POLL_IN);
                                        }
#endif
                                }
				/*device available */
				//gf_dev->device_available = 1;
			}
			break;
		default:
			pr_info("%s defalut\n", __func__);
			break;
		}
	}
	return NOTIFY_OK;
}

static struct notifier_block goodix_noti_block = {
	.notifier_call = goodix_fb_state_chg_callback,
};

static void gf_reg_key_kernel(struct fp_device_data *gf_dev)
{
    int i;

    set_bit(EV_KEY, gf_dev->input->evbit); //tell the kernel is key event
    for(i = 0; i< ARRAY_SIZE(key_map); i++) {
        set_bit(key_map[i].report_val, gf_dev->input->keybit);
    }

    gf_dev->input->name = GF_INPUT_NAME;
    if (input_register_device(gf_dev->input))
        pr_warn("Failed to register GF as input device.\n");
}

/* --- goodix operate zone --- */

/* +++ common operate zone +++ */

static void clean_touch_mask(unsigned long ptr) {
	printk("[JK] Clean touch mask \n");
	g_FP_Disable_Touch = false;

	return;
}

static void init_FpMaskTouch_Timer(struct fp_device_data *gf_dev)
{

	unsigned long expires;

	printk("[JK] Init Disable touch timer\n");

	if (!gf_dev->FpTimer_expires) {
		init_timer(&gf_dev->FpMaskTouch_Timer);
		gf_dev->FpMaskTouch_Timer.function = clean_touch_mask;
		gf_dev->FpMaskTouch_Timer.expires = jiffies + msecs_to_jiffies(500);
		add_timer(&gf_dev->FpMaskTouch_Timer);
		gf_dev->FpTimer_expires = gf_dev->FpMaskTouch_Timer.expires;
	} else {
		expires = jiffies + msecs_to_jiffies(500);
		if (!expires)
			expires = 1;

		if (!gf_dev->FpTimer_expires || time_after(expires, gf_dev->FpTimer_expires)) {
			mod_timer(&gf_dev->FpMaskTouch_Timer, expires);
			gf_dev->FpTimer_expires = expires;
		}
	}

	return;
}

static void del_FpMaskTouch_Timer(struct fp_device_data *gf_dev)
{
	del_timer(&gf_dev->FpMaskTouch_Timer);
	gf_dev->FpTimer_expires = 0;
	g_FP_Disable_Touch = false;
	printk("[JK] Del Disable touch timer\n");
	return;
}

/* +++ ASUS proc fingerprint Interface +++ */
static int fingerpring_proc_read(struct seq_file *buf, void *v)
{

	if(g_module_vendor == vendor_module_gdix_5206) {
		seq_printf(buf, "%s\n", "5206");
		return 0;
	} else if (g_module_vendor == vendor_module_gdix_5216) {
		seq_printf(buf, "%s\n", "5216");
		return 0;
	} else {
		seq_printf(buf, "%s\n", "NA");
		return 0;
	}

}
static int fingerprint_proc_open(struct inode *inode, struct  file *file)
{
    return single_open(file, fingerpring_proc_read, NULL);
}

static void create_fingerprint_proc_file(void)
{
	static const struct file_operations proc_fops = {
		.owner = THIS_MODULE,
		.open =  fingerprint_proc_open,
		.read = seq_read,
		.release = single_release,
	};

	struct proc_dir_entry *proc_file = proc_create("fpmod", 0444, NULL, &proc_fops);

	if (!proc_file) {
		printk("[PF]%s failed!\n", __FUNCTION__);
	}
	return;
}

/* --- ASUS proc fingerprint Interface --- */
#if 0
static bool check_elan_module(struct fp_device_data *pdata)
{
	char tx_buf[64] = {1};
	char rx_buf[64] = {1};
	struct spi_transfer t;
	struct spi_message m;
	int i, j = 0;

	printk("spi_test_transfer start ! \n");
	for (j = 0; j < 2; j++) {
		mdelay(20);
		for (i=0; i<6; i++) {
			printk("%0x ", rx_buf[i]);
		}
		printk("\n");
		tx_buf[0] = 0xC1;	//EP0 Read
		tx_buf[1] = 1;
		pdata->spi->bits_per_word = 8;
		pdata->spi->mode  = SPI_MODE_0 ;
		memset(&t, 0, sizeof(t));
		t.tx_buf = tx_buf;
		t.rx_buf = rx_buf;
		t.len = 6;

		printk("[jacob] spi_message_init ! \n");
		spi_message_init(&m);
		printk("[jacob] spi_message_add_tail! \n");
		spi_message_add_tail(&t, &m);
		printk("[jacob] spi_message_add_tail end ! \n");
		printk("spi sync return %d \n", spi_sync(pdata->spi, &m));

	}

	if(0xbf == rx_buf[3] && 0x37 == rx_buf[5]) {
		IMG_WIDTH = (unsigned int)(rx_buf[5] - rx_buf[4] + 1);
		IMG_HEIGHT = (unsigned int)(rx_buf[3] - rx_buf[2] + 1);
		IMG_WIDTH_DEFAULT = IMG_WIDTH;
		IMG_HEIGHT_DEFAULT = IMG_HEIGHT;

		return true;
	} else {
		return false;
	}

}

static bool check_syna_module(struct fp_device_data *pdata)
{
	char tx_buf[64] = {1};
	char rx_buf[64] = {1};
	struct spi_transfer t;
	struct spi_message m;
	int i, j = 0;

	mdelay(20);

	printk("spi_test_transfer start ! \n");
	for (j = 0; j < 2; j++) {
		for (i=0; i<6; i++) {
			printk("%0x ", rx_buf[i]);
		}
		printk("\n");
		tx_buf[0] = 1;	//EP0 Read
		tx_buf[1] = 0;
		pdata->spi->bits_per_word = 8;
		pdata->spi->mode  = SPI_MODE_0 ;
		pdata->spi->max_speed_hz = MAX_BAUD_RATE;
		memset(&t, 0, sizeof(t));
		t.tx_buf = tx_buf;
		t.rx_buf = rx_buf;
		t.len = 6;

		printk("[jacob] spi_message_init ! \n");
		spi_message_init(&m);
		printk("[jacob] spi_message_add_tail! \n");
		spi_message_add_tail(&t, &m);
		printk("[jacob] spi_message_add_tail end ! \n");
		printk("spi sync return %d \n", spi_sync(pdata->spi, &m));

		for (i=0; i<6; i++) {
			printk("%0x ", rx_buf[i]);
		}
		printk("\n");
		mdelay(10);

		printk("spi_test_transfer start  01 \n");

		tx_buf[0] = 1;	//EP0 Read
		tx_buf[1] = 0;

		memset(&t, 0, sizeof(t));
		t.tx_buf = tx_buf;
		t.rx_buf = rx_buf;
		t.len = 6;

		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		printk("spi sync return %d \n", spi_sync(pdata->spi, &m));

		for (i=0; i<6; i++) {
			printk("%0x ", rx_buf[i]);
		}
		printk("\n");
		mdelay(5);

		printk("spi_test_transfer start  02 \n");

		tx_buf[0] = 2;	//EP0 Read
		tx_buf[1] = 1;

		memset(&t, 0, sizeof(t));
		t.tx_buf = tx_buf;
		t.rx_buf = rx_buf;
		t.len = 2;

		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		spi_sync(pdata->spi, &m);

		mdelay(5);

		printk("spi_test_transfer start  03 \n");

		tx_buf[0] = 3;	//EP0 Read
		tx_buf[1] = 0;

		memset(&t, 0, sizeof(t));
		t.tx_buf = tx_buf;
		t.rx_buf = rx_buf;
		t.len = 40;

		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		printk("spi_test_transfer start  03 \n");
		printk("spi sync return %d \n", spi_sync(pdata->spi, &m));

		for (i=0; i<40; i++) {
			printk("%0x ", rx_buf[i]);
		}
		printk("\n");
	}

	if((0xff == rx_buf[0] || 0x7f == rx_buf[0]) && 0xff == rx_buf[1] && 0x0 == rx_buf[2] && 0x0 == rx_buf[3]) {
		return true;
	} else {
		return false;
	}

}

/* vfsspi pinctrol */
static int fp_set_pinctrl(struct fp_device_data *pdata, bool active)
{
	int ret;
	if (active) { /* Change to active settings */
		ret = pinctrl_select_state(pdata->pinctrl, pdata->pins_active);
	}else {
		ret = pinctrl_select_state(pdata->pinctrl, pdata->pins_sleep);
	}

	if (ret)
		dev_err(&pdata->spi->dev, "%s: pinctrl_select_state ret:%d Setting:%d\n", __func__, ret, active);

	return ret;
}
/* vfsspi pinctrol */

/* vfsspi_set_clks */
static int fp_set_clks(struct fp_device_data *pdata, bool enable)
{
	int ret = 0;
	if (enable) {
		if (!fp_set_clks_flag) {
			/* Enable the spi clocks */
			ret = clk_set_rate(pdata->core_clk, pdata->spi->max_speed_hz); 
			if (ret) {
				dev_err(&pdata->spi->dev, "%s: Error setting clk_rate:%d\n", __func__, pdata->spi->max_speed_hz);
			}
			ret = clk_prepare_enable(pdata->core_clk);
			if (ret) {
				dev_err(&pdata->spi->dev, "%s: Error enabling core clk\n",  __func__);
			}
			ret = clk_prepare_enable(pdata->iface_clk);
			if (ret) {
				dev_err(&pdata->spi->dev, "%s: Error enabling iface clk\n", __func__);
			}
			fp_set_clks_flag = true;
		}
	} else {
		if (fp_set_clks_flag) {
			/* Disable the clocks */
			clk_disable_unprepare(pdata->iface_clk);
			clk_disable_unprepare(pdata->core_clk);
			fp_set_clks_flag = false;
		}
	}
	return ret; 
}
/* vfsspi_set_clks */

/* fvsspi_change_pipe_owner */
static int fp_change_pipe_owner(struct fp_device_data *pdata, bool to_tz)
{
	/* CMD ID collected from tzbsp_blsp.c to change Ownership */
	const u32 TZ_BLSP_MODIFY_OWNERSHIP_ID = 3;
	const u32 TZBSP_APSS_ID = 1;
	const u32 TZBSP_TZ_ID = 3 ;
	struct scm_desc desc; //scm call descriptor
	int ret;
	/* CMD ID to change BAM PIPE Owner*/
	desc.arginfo = SCM_ARGS(2); //# of arguments
	desc.args[0] = pdata->qup_id; //BLSPID (1-12)
	desc.args[1] = (to_tz) ? TZBSP_TZ_ID : TZBSP_APSS_ID; //Owner if TZ or Apps
	ret = scm_call2(SCM_SIP_FNID(SCM_SVC_TZ, TZ_BLSP_MODIFY_OWNERSHIP_ID), &desc);
	return (ret || desc.ret[0]) ? -1 : 0;
}
/* fvsspi_change_pipe_owner */

/* fvsspi_set_fabric */
static int fp_set_fabric(struct fp_device_data *pdata, bool active)
{
	int ret;
	struct spi_master *master = pdata->spi->master;
	
	if (active) {
		if (!fp_set_fabric_flag) {
			ret = master->prepare_transfer_hardware(master);
			fp_set_fabric_flag = true;
		}
	}else {
		if (fp_set_fabric_flag) {
			ret = master->unprepare_transfer_hardware(master);
			fp_set_fabric_flag = false;
		}
	}
	return ret;
}
/* fvsspi_set_fabric */
#endif

/* +++ ASUS add for suspend/resume +++ */
static int fp_sensor_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct fp_device_data *fp = dev_get_drvdata(&pdev->dev);

		printk("[FP] sensor suspend +++\n");
//		bState = 0;


		if (fp->is_drdy_irq_enabled & !fp->irq_wakeup_flag & !g_Charger_mode) {
			printk("[FP] enable irq wake up  \n");
			enable_irq_wake(fp->isr_pin);
			fp->irq_wakeup_flag = true;
		}


		printk("[FP] sensor suspend ---\n");
	
	return 0;
}

static int fp_sensor_resume(struct platform_device *pdev)
{
	struct fp_device_data *fp = dev_get_drvdata(&pdev->dev);

		printk("[FP] sensor resume +++\n");

		if (fp->irq_wakeup_flag) {
			disable_irq_wake(fp->isr_pin);
			fp->irq_wakeup_flag = false;
		}

#if 0
		if(InNW) {
			printk("[FP] sensor resume to NW \n");
			fp_set_pinctrl(fp, 1);
			fp_set_fabric(fp, 1);
			fp_set_clks(fp, 1);
			fp_change_pipe_owner(fp, 0);
		} else {
			printk("[FP] sensor resume to QSEE \n");
			fp_set_pinctrl(fp, 1);
			fp_set_fabric(fp, 1);
			fp_set_clks(fp, 1);
			fp_change_pipe_owner(fp, 1);
		}
#endif
//		bState = 1;
		
//		if (g_module_vendor == 2) {
//			elan_nw_module(fp);
//		}
//		gpio_set_value(fp->sleep_pin, 1);
		printk("[FP] sensor resume ---\n");
		
	return 0;
}

/* --- ASUS add for suspend/resume  --- */


/* +++ change excute mode +++ */
#if 0
static int fp_change_excute_mode(struct fp_device_data *pdata, bool tz)
{

	printk("[FP] fp_change_excute_mode = %d !\n", tz);
	if (tz) {	
		fp_set_pinctrl(pdata, 1);
		fp_set_fabric(pdata, 1);

		fp_set_clks(pdata, 1);
		fp_change_pipe_owner(pdata, 1);
		InNW = false;
	} else {
		fp_set_pinctrl(pdata, 1);
		fp_set_fabric(pdata, 1);

		fp_set_clks(pdata, 1);
		fp_change_pipe_owner(pdata, 0);
		InNW = true;
	}
	return 0;
	

}
#endif
/* --- change excute mode --- */

/* +++ fp NW/TEE evn transfer +++ */
#if 0
static int fp_transfer_init(struct spi_device *spi,
			struct fp_device_data *pdata, bool on)
{

	int status = 0;

	if (on) {

		/* pinctrol part*/
		printk("[Jacob] vfsspi Get the pinctrl node \n");
		/* Get the pinctrl node */
		pdata->pinctrl = devm_pinctrl_get(&spi->dev);
		if (IS_ERR_OR_NULL(pdata->pinctrl)) {
		     dev_err(&spi->dev, "%s: Failed to get pinctrl\n", __func__);
		     return PTR_ERR(pdata->pinctrl);
		}
		
		printk("[Jacob] vfsspi Get the active setting \n");
		/* Get the active setting */
		pdata->pins_active = pinctrl_lookup_state(pdata->pinctrl,
		                            "spi_default");
		if (IS_ERR_OR_NULL(pdata->pins_active)) {
			dev_err(&spi->dev, "%s: Failed to get pinctrl state active\n", __func__);
			return PTR_ERR(pdata->pins_active);
		}

		printk("[Jacob] vfsspi Get the sleep setting \n");
		/* Get sleep settings */
		pdata->pins_sleep = pinctrl_lookup_state(pdata->pinctrl,
		                            "spi_sleep");
		if (IS_ERR_OR_NULL(pdata->pins_sleep)) {
			dev_err(&spi->dev, "%s: Failed to get pinctrl state sleep\n", __func__);
		return PTR_ERR(pdata->pins_sleep);
		}
		/* pinctrol part*/

		/* clock part */
		printk("[Jacob] vfsspi Get iface_clk info \n");
		/* Get iface_clk info */
		pdata->iface_clk = clk_get(&spi->dev, "vfsiface_clk");
		if (IS_ERR(pdata->iface_clk)) {
			dev_err(&spi->dev, "%s: Failed to get iface_clk %ld\n", __func__, PTR_ERR(pdata->iface_clk));
			return PTR_ERR(pdata->iface_clk);
		}
		printk("[Jacob] vfsspi Get core_clk info \n");
		
		/* Get core_clk info */
		pdata->core_clk = clk_get(&spi->dev, "vfscore_clk");
		if (IS_ERR(pdata->core_clk)) {
			dev_err(&spi->dev, "%s: Failed to get core_clk %p\n", __func__, pdata->core_clk);
			return PTR_ERR(pdata->core_clk);
		}
		/* clock part */

		printk("[Jacob] vfsspi Get QUP ID \n");
		/* Get the QUP ID (#1-12) */
		status = of_property_read_u32(spi->dev.of_node, "qcom,qup-id", &pdata->qup_id);
		if (status) {
			dev_err(&spi->dev, "Error getting qup_id\n");
			return status;
		}

		/* Grab SPI master lock for exclusive access
		call spi_bus_unlock to unlock the lock. */
		//	spi_bus_lock(spi->master);
		printk("[Jacob] vfsspi Get QUP ID end\n");
	}
	return 0;
}
#endif
/* --- fp NW/TEE evn transfer --- */

/* +++ fp gpio init +++ */
static int fp_gpio_init(struct fp_device_data *pdata)
{

	int err = 0;
	/* request part */
	if (gpio_request(pdata->drdy_pin, "fp_drdy") < 0) {
		err = -EBUSY;
		printk("[FP][%s] gpio request failed ! \n", __func__);
		return err;		
	}

	if (gpio_request(pdata->sleep_pin, "fp_sleep")) {
		err = -EBUSY;
		printk("[FP][%s] gpio request failed ! \n", __func__);
		goto fp_gpio_init_sleep_pin_failed;
	}

	if (gpio_request(pdata->osvcc_Titan_enable_pin, "fp_Titan_pwr_en")) {
		err = -EBUSY;
		printk("[FP][%s] gpio request fp_Titan_pwr_e failed ! \n", __func__);
		goto fp_gpio_init_titan_vcc_pin_failed;
	}

	/* config part */
	err = gpio_direction_output(pdata->sleep_pin, 1);
	if (err < 0) {
		printk("[FP][%s] gpio_direction_output SLEEP failed ! \n", __func__);
		err = -EBUSY;
		goto fp_gpio_config_failed;
	}

	err = gpio_direction_output(pdata->osvcc_Titan_enable_pin, 1);
	if (err < 0) {
		printk("[FP][%s] gpio_direction_output osvcc_enable_pin failed ! \n", __func__);
		err = -EBUSY;
		goto fp_gpio_config_failed;
	}
	/* ---no use in msm8953--- */

	return err;
fp_gpio_config_failed:
	gpio_free(pdata->osvcc_Titan_enable_pin);
fp_gpio_init_titan_vcc_pin_failed:
	gpio_free(pdata->sleep_pin);
fp_gpio_init_sleep_pin_failed:
	gpio_free(pdata->drdy_pin);

	return err;
}
/* --- fp gpio init --- */

#if 0
/* +++ fp power on/off +++ */
static int fp_power_on(struct fp_device_data *pdata, bool on)
{
	int rc;

	if (!on)
		goto power_off;
#if 0
	rc = regulator_enable(pdata->sovcc);
	if (rc) {
		printk("[FP][%s]Regulator sovcc enable failed rc=%d\n", __func__, rc);
		return rc;
	}
#endif
	rc = regulator_enable(pdata->vcc);
	if (rc) {
		printk("[FP][%s]Regulator vcc enable failed rc=%d\n", __func__, rc);
		regulator_disable(pdata->sovcc);
	}

	return rc;

power_off:
#if 0
	rc = regulator_disable(pdata->sovcc);
	if (rc) {
		printk("[FP][%s]Regulator sovcc disable failed rc=%d\n", __func__, rc);
		return rc;
	}
#endif
	rc = regulator_disable(pdata->vcc);
	if (rc) {
		printk("[FP][%s]Regulator vcc disable failed rc=%d\n", __func__, rc);
		rc = regulator_enable(pdata->sovcc);
		if (rc) {
		printk("[FP][%s]Regulator sovcc disable failed rc=%d\n", __func__, rc);
		}
	}

	return rc;
}
/* --- fp power on/off --- */
#endif
#if 0
/* +++ fp power init +++ */
static int fp_power_init(struct device *dev,
			struct fp_device_data *pdata, bool on)
{
	int rc;

	if (!on)
		goto pwr_deinit;
	/* +++ pars regulator+++ */

	pdata->sovcc = devm_regulator_get(dev, "sovcc");
	if (IS_ERR( pdata->sovcc)) {
		rc = PTR_ERR(pdata->sovcc);
		printk("Regulator get VFS sovcc  failed rc=%d\n", rc);
		return rc;
		}

	if (regulator_count_voltages(pdata->sovcc) > 0) {
		rc = regulator_set_voltage(pdata->sovcc, 3300000,
					   3300000);
		if (rc) {
			printk("Regulator set sovcc failed vdd rc=%d\n", rc);
			goto reg_sovcc_put;
		}
	}

	pdata->vcc = devm_regulator_get(dev, "vcc");
	if (IS_ERR( pdata->vcc)) {
		rc = PTR_ERR(pdata->vcc);
		printk("Regulator get vcc failed rc=%d\n", rc);
		goto reg_vdd_set_vtg;
		}

	if (regulator_count_voltages(pdata->vcc) > 0) {
		rc = regulator_set_voltage(pdata->vcc, 1800000,
					   1800000);
		if (rc) {
			printk("Regulator set_vcc failed vdd rc=%d\n", rc);
			goto reg_vcc_i2c_put;
		}
	}
	/* +++ pars regulator+++ */
	return 0;
reg_vcc_i2c_put:
	regulator_put(pdata->vcc);
reg_vdd_set_vtg:
	if (regulator_count_voltages(pdata->sovcc) > 0)
		regulator_set_voltage(pdata->sovcc, 0, voltage_supply[1]);
//reg_sovcc_put:
//	regulator_put(pdata->sovcc);
//	return rc;

pwr_deinit:
//	if (regulator_count_voltages(pdata->sovcc) > 0)
//		regulator_set_voltage(pdata->sovcc, 0, 3300000);

//	regulator_put(pdata->sovcc);

	if (regulator_count_voltages(pdata->vcc) > 0)
		regulator_set_voltage(pdata->vcc, 0, voltage_supply[1]);

	regulator_put(pdata->vcc);
	return 0;
}
/* --- fp power init --- */
#endif
/* +++ fp check vendor +++ */
static int fp_check_vendor(struct fp_device_data *pdata)
{

	int status = 0;
	int id1 = 0;

	id1 = gpio_get_value(pdata->FP_ID1);
/*  id2 = gpio_get_value(pdata->FP_ID2); */         
/*	status = id1;

	if (status) {
		status = vendor_module_gdix_5206;
	} else {
		status = vendor_module_gdix_5216;
	}*/

	//ASUS_BSP YuSiang: Fix module to vendor_module_gdix_5216 for ZE554KL
	status = vendor_module_gdix_5216;
	printk("[Jacob] ID1 = %d  \n",  id1);
	printk("[Jacob] status = %d  \n",  status);

	return status;
}

/* --- fp check vendor --- */

/* +++ fp_id gpio init +++ */
static int fp_check_gpio_init(struct fp_device_data *pdata)
{

	int err = 0;
	/* request part */
	if (gpio_request(pdata->FP_ID1, "fp_id1") < 0) {
		err = -EBUSY;
		printk("[FP][%s] gpio request fp_id1 failed ! \n", __func__);
		return err;		
	}

	/* config part */
	err = gpio_direction_output(pdata->FP_ID1, 1);
	if (err < 0) {
		printk("[FP][%s] gpio_direction_output fp_id1 failed ! \n", __func__);
		err = -EBUSY;
		goto fp_gpio_init_id1_pin_failed;
	}

	return err;

fp_gpio_init_id1_pin_failed:
	gpio_free(pdata->FP_ID1);

	return err;
}
/* --- fp_id gpio init --- */

/* vfsspidri pars dt data*/
static int fp_pars_dt(struct device *dev,
			struct fp_device_data *pdata)
{

	struct device_node *np = dev->of_node;
	
	/* +++reset, irq gpio info+++ */
	pdata->sleep_pin = of_get_named_gpio_flags(np, "asus-fp,sleep-gpio", 0, NULL);
	if (pdata->sleep_pin < 0)
		return pdata->sleep_pin;

	pdata->drdy_pin = of_get_named_gpio_flags(np, "asus-fp,irq-gpio", 0, NULL);
	if (pdata->drdy_pin < 0)
		return pdata->drdy_pin;
	/* ---reset, irq gpio info --- */

	/* +++ 3.3 enable gpio info need config in ER stage +++ */

	pdata->osvcc_Titan_enable_pin = of_get_named_gpio(np, "asus-fp,vcc-enable-gpio", 0);
	printk("[FP] AAAAPpwe_pin = %d \n",pdata->osvcc_Titan_enable_pin);
	if (pdata->osvcc_Titan_enable_pin < 0){
		printk("[FP] QQQQQQQQQPpwe_pin = %d < 0",pdata->osvcc_Titan_enable_pin);
		pdata->osvcc_Titan_enable_pin = 0;
	}
    
	/* +++ Fingerprint ID pin  +++ */
	pdata->FP_ID1 = of_get_named_gpio_flags(np, "asus-fp,ID1-gpio", 0, NULL);
	if (pdata->FP_ID1 < 0) {
            printk("[JK] pard FP ID 1 fail ! \n");
            return pdata->FP_ID1;
        }

	printk("[FP] sleep_pin = %d, drdy_pin = %d, Ppwe_pin = %d , ID1 = %d! \n", pdata->sleep_pin, pdata->drdy_pin, pdata->osvcc_Titan_enable_pin, pdata->FP_ID1);

	return 0;
}
/* vfsspidri pars dt data*/

/* --- common operate zone --- */

static int fp_sensor_probe(struct platform_device *pdev)
{
	int status = 0;
	struct fp_device_data *fp_device;
	struct device *dev = &pdev->dev;
//	unsigned long minor;
	int ret;

	printk("[FP] Probe +++\n");
	fp_device = kzalloc(sizeof(*fp_device), GFP_KERNEL);
	if (fp_device == NULL) {
		printk("[FP] alloc Fingerprint data fail.\r\n");
		status = -1;
		goto fp_kmalloc_failed;
	}
	/* Inin status */
	fp_device->module_vendor = 0;
	fp_device->irq_wakeup_flag = false;
	fp_device->FpTimer_expires = 0;
	fp_device->enable_touch_mask = 0;
    fp_device->FP_ID2 = 0;
	/* Inin status */

	/*status = fp_pars_dt(&pdev->dev, fp_device);*/

	if (fp_pars_dt(&pdev->dev, fp_device) < 0) {
		printk("[FP] opps pars gpio fail ! \n");
		status = -2;
		goto fp_pars_dt_failed;
	}
            
	if (fp_check_gpio_init(fp_device) < 0) {
		printk("[FP] opps init id gpio ! \n");
		status = -2;
		goto fp_pars_dt_failed;
	}

	fp_device->module_vendor = fp_check_vendor(fp_device);

	if (fp_gpio_init(fp_device)) {
		printk("[FP] opps fp_gpio_init fail ! \n");
		status = -5;
		goto fp_pars_dt_failed;
	}

	msleep(15);

	dev_set_drvdata(&pdev->dev, fp_device);

	create_fingerprint_proc_file();


	ret = gpio_direction_output(fp_device->FP_ID1, 0);
	if (ret < 0) {
		printk("[FP][%s] pull down fp_id1 failed ! \n", __func__);
		status = -6;
		goto fp_pars_dt_failed;
	}

#if 0
	ret = gpio_direction_output(fp_device->FP_ID2, 0);
	if (ret < 0) {
		printk("[FP][%s] pull down fp_id2 failed ! \n", __func__);
		status = -6;
		goto fp_pars_dt_failed;
	}
#endif
	wake_lock_init(&fp_device->wake_lock, WAKE_LOCK_SUSPEND, "FP_wake_lock");

	switch(fp_device->module_vendor) {
		case vendor_module_syna:
			/* +++ switch FP vdndor +++ */
			spin_lock_init(&fp_device->vfs_spi_lock);
			mutex_init(&fp_device->buffer_mutex);
			mutex_init(&fp_device->kernel_lock);

			INIT_LIST_HEAD(&fp_device->device_entry);

			status = gpio_direction_input(fp_device->drdy_pin);
			if (status < 0) {
				pr_err("gpio_direction_input DRDY failed, status = %d \n", status);
				status = -8;
				goto fp_pars_dt_failed;
			}

			gpio_irq = gpio_to_irq(fp_device->drdy_pin);
			fp_device->isr_pin = gpio_irq;
			if (gpio_irq < 0) {
				pr_err("gpio_to_irq failed\n");
				status = -9;
				goto fp_pars_dt_failed;
			}

			if (request_irq(gpio_irq, vfsspi_irq, IRQF_TRIGGER_RISING,
					"vfsspi_irq", fp_device) < 0) {
				pr_err("request_irq failed \n");
				status = -10;
				goto fp_pars_dt_failed;
			}

			fp_device->is_drdy_irq_enabled = DRDY_IRQ_ENABLE;

			mutex_lock(&device_list_mutex);
			/* Create device node */
			/* register major number for character device */
			status = alloc_chrdev_region(&(fp_device->devt),
						     0, 1, VALIDITY_PART_NAME);
			if (status < 0) {
				pr_err("alloc_chrdev_region failed\n");
				goto fp_probe_alloc_chardev_failed;
			}

			cdev_init(&(fp_device->cdev), &vfsspi_fops);
			fp_device->cdev.owner = THIS_MODULE;
			status = cdev_add(&(fp_device->cdev), fp_device->devt, 1);
			if (status < 0) {
				pr_err("cdev_add failed\n");
				unregister_chrdev_region(fp_device->devt, 1);
				goto fp_probe_cdev_add_failed;
			}

			vfsspi_device_class = class_create(THIS_MODULE, "validity_fingerprint");

			if (IS_ERR(vfsspi_device_class)) {
				pr_err("vfsspi_init: class_create() is failed - unregister chrdev.\n");
				cdev_del(&(fp_device->cdev));
				unregister_chrdev_region(fp_device->devt, 1);
				status = PTR_ERR(vfsspi_device_class);
				goto vfsspi_probe_class_create_failed;
			}

			dev = device_create(vfsspi_device_class, &pdev->dev,
					    fp_device->devt, fp_device, "vfsspi");
			status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
			if (status == 0)
				list_add(&fp_device->device_entry, &device_list);
			mutex_unlock(&device_list_mutex);

			if (status != 0)
				goto vfsspi_probe_failed;

			//enable_irq_wake(gpio_irq);

			#if 0
						status = sysfs_create_group(&spi->dev.kobj, &vfsspi_attribute_group);
						if (0 != status) {
							printk("[FP][FP_ERR] %s() - ERROR: sysfs_create_group() failed.error code: %d\n", __FUNCTION__, status);
							sysfs_remove_group(&spi->dev.kobj, &vfsspi_attribute_group);
							return -EIO;
						} else {
							printk("[FP][FP_ERR] %s() - sysfs_create_group() succeeded. \n",  __FUNCTION__);
						}
			#endif
			/* --- switch FP vdndor --- */
			break;
		case vendor_module_gdix_316m:
		case vendor_module_gdix_5116m:
		case vendor_module_gdix_3266A:
                case vendor_module_gdix_5206:
                case vendor_module_gdix_5216:                
			/* Claim our 256 reserved device numbers.  Then register a class
			 * that will key udev/mdev to add/remove /dev nodes.  Last, register
			 * the driver which manages those device numbers.
			*/


			INIT_LIST_HEAD(&fp_device->device_entry);
#if defined(USE_SPI_BUS)
			fp_device->spi = spi;
#elif defined(USE_PLATFORM_BUS)
			fp_device->spi = pdev;
#endif
			fp_device->irq_gpio = fp_device->drdy_pin;
			fp_device->reset_gpio = fp_device->sleep_pin;
			fp_device->pwr_gpio = fp_device->osvcc_Titan_enable_pin;
			fp_device->device_available = 0;
			fp_device->fb_black = 0;

			/* jacob pinctrol part*/
			printk("[Jacob] vfsspi Get the pinctrl node \n");
			/* Get the pinctrl node */
			fp_device->pinctrl = devm_pinctrl_get(&pdev->dev);
			if (IS_ERR_OR_NULL(fp_device->pinctrl)) {
			     dev_err(&pdev->dev, "%s: Failed to get pinctrl\n", __func__);
			     return PTR_ERR(fp_device->pinctrl);
			}
			
			printk("[Jacob] vfsspi Get the active setting \n");
			/* Get the active setting */
			fp_device->pins_active = pinctrl_lookup_state(fp_device->pinctrl,
			                            "fp_default");
			if (IS_ERR_OR_NULL(fp_device->pins_active)) {
				dev_err(&pdev->dev, "%s: Failed to get pinctrl state active\n", __func__);
				return PTR_ERR(fp_device->pins_active);
			}

			printk("[Jacob] vfsspi Get the sleep setting \n");
			/* Get sleep settings */
			fp_device->pins_sleep = pinctrl_lookup_state(fp_device->pinctrl,
			                            "fp_sleep");
			if (IS_ERR_OR_NULL(fp_device->pins_sleep)) {
				dev_err(&pdev->dev, "%s: Failed to get pinctrl state sleep\n", __func__);
			return PTR_ERR(fp_device->pins_sleep);
			}

			mutex_lock(&device_list_lock);
			/* Create device node */
			/* register major number for character device */
			status = alloc_chrdev_region(&(fp_device->devt),
						     0, 1, GF_DEV_NAME);
			if (status < 0) {
				pr_err("alloc_chrdev_region failed\n");
				goto fp_probe_alloc_chardev_failed;
			}

			cdev_init(&(fp_device->cdev), &gf_fops);
			fp_device->cdev.owner = THIS_MODULE;
			status = cdev_add(&(fp_device->cdev), fp_device->devt, 1);
			if (status < 0) {
				pr_err("cdev_add failed\n");
				unregister_chrdev_region(fp_device->devt, 1);
				goto fp_probe_cdev_add_failed;
			}

			gf_class = class_create(THIS_MODULE, GF_DEV_NAME);

			if (IS_ERR(gf_class)) {
				pr_err("vfsspi_init: class_create() is failed - unregister chrdev.\n");
				cdev_del(&(fp_device->cdev));
				unregister_chrdev_region(fp_device->devt, 1);
				status = PTR_ERR(gf_class);
				goto vfsspi_probe_class_create_failed;
			}

			dev = device_create(gf_class, &pdev->dev,
					    fp_device->devt, fp_device, GF_DEV_NAME);
			status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
			if (status == 0)
				list_add(&fp_device->device_entry, &device_list);

#if 0
			minor = find_first_zero_bit(minors, N_SPI_MINORS);
			if (minor < N_SPI_MINORS) {
				struct device *dev;
				fp_device->devt = MKDEV(SPIDEV_MAJOR, minor);
				dev = device_create(gf_class, &fp_device->spi->dev, fp_device->devt,
						    fp_device, GF_DEV_NAME);
				status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
			} else {
				dev_dbg(&fp_device->spi->dev, "no minor number available!\n");
				status = -ENODEV;
			}

			if (status == 0) {
				set_bit(minor, minors);
				list_add(&fp_device->device_entry, &device_list);
			} else {
				fp_device->devt = 0;
			}
#endif
			mutex_unlock(&device_list_lock);

			if (status == 0) {
				/*input device subsystem */
				fp_device->input = input_allocate_device();
				if (fp_device->input == NULL) {
					dev_dbg(&fp_device->input->dev,"Faile to allocate input device.\n");
					status = -ENOMEM;
				}
		#ifdef AP_CONTROL_CLK
				dev_info(&fp_device->spi->dev,"Get the clk resource.\n");
				/* Enable spi clock */
				if (gfspi_ioctl_clk_init(fp_device))
					goto gfspi_probe_clk_init_failed;

				if (gfspi_ioctl_clk_enable(fp_device))
					goto gfspi_probe_clk_enable_failed;

				spi_clock_set(fp_device, 1000000);
		#endif

				fp_device->notifier = goodix_noti_block;
				fb_register_client(&fp_device->notifier);
				gf_reg_key_kernel(fp_device);
				
		        fp_device->irq = gf_irq_num(fp_device);

		        printk("[Goodix] irq num = %d \n", fp_device->irq);
		#if 1
				ret = request_threaded_irq(fp_device->irq, NULL, gf_irq,
							   IRQF_TRIGGER_RISING | IRQF_ONESHOT,
							   "gf", fp_device);
		#else
				ret = request_irq(gf_dev->irq, gf_irq,
		                       IRQ_TYPE_EDGE_RISING, /*IRQ_TYPE_LEVEL_HIGH,*/
		                       "gf", gf_dev);
		#endif
				if (!ret) {
					fp_device->irq_enabled = 1;
					if (!g_Charger_mode)
						enable_irq_wake(fp_device->irq);
					gf_disable_irq(fp_device);
					printk("[Jacob] disable irq \n");
				}
			}

	status = pinctrl_select_state(fp_device->pinctrl, fp_device->pins_active);

	printk("[Jacob] report status = %d \n", status);


			pr_info(" status = 0x%x\n", status);
//			&gf = fp_device;
	break;
	}

	g_module_vendor = fp_device->module_vendor;

	printk("[FP] change owner ship end! \n");
	
	return 0;


vfsspi_probe_failed:
vfsspi_probe_class_create_failed:
fp_probe_cdev_add_failed:
fp_probe_alloc_chardev_failed:
fp_pars_dt_failed:
	kfree(fp_device);
fp_kmalloc_failed:
	printk("[FP] Probe failed wieh error code %d ! \n", status);
	return status;
}


static int fp_sensor_remove(struct platform_device *pdev)
{
	return 0;
}

static struct of_device_id fp_match_table[] = {
	{ .compatible = "asus,fingerprint",},
	{ },
};

static struct platform_driver fp_sensor_driver = {
	.driver = {
		.name = "fphandle",
		.owner = THIS_MODULE,
		.of_match_table = fp_match_table,
	},
	.probe         	= fp_sensor_probe,
	.remove			= fp_sensor_remove,
	.suspend		= fp_sensor_suspend,
	.resume			= fp_sensor_resume,
};


static int __init fp_sensor_init(void)
{	
	int err = 0;
	printk("[FP] Driver INIT +++\n");
#if 0
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	err = register_chrdev(SPIDEV_MAJOR, CHRD_DRIVER_NAME, &gf_fops);
	if (err < 0) {
		pr_warn("Failed to register char device!\n");
		FUNC_EXIT();
		return err;
	}
	gf_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(gf_class)) {
		unregister_chrdev(SPIDEV_MAJOR, fp_sensor_driver.driver.name);
		pr_warn("Failed to create class.\n");
		FUNC_EXIT();
		return PTR_ERR(gf_class);
	}
#endif
	err = platform_driver_register(&fp_sensor_driver);
	if (err != 0) {
		printk("[FP] platform_driver_register fail, Error : %d\n", err);
	}
#if 0
	if (err < 0) {
		class_destroy(gf_class);
		unregister_chrdev(SPIDEV_MAJOR, fp_sensor_driver.driver.name);
		pr_warn("Failed to register SPI driver.\n");
	}
#endif
#ifdef GF_NETLINK_ENABLE
            if (g_module_vendor == vendor_module_gdix_3266A) {
                netlink_init();
            }
#endif

	printk("[FP] Driver INIT ---\n");

	return err;
}

static void __exit fp_sensor_exit(void)
{
	printk("[FP] Driver EXIT +++\n");
#ifdef GF_NETLINK_ENABLE
        if (g_module_vendor == vendor_module_gdix_3266A) {
            netlink_exit();
        }
#endif
	platform_driver_unregister(&fp_sensor_driver);	
	
	printk("[FP] Driver EXIT ---\n");
}


module_init(fp_sensor_init);
module_exit(fp_sensor_exit);

MODULE_AUTHOR("jacob_kung <jacob_kung@asus.com>");
MODULE_DESCRIPTION("asus fingerprint handle");
MODULE_LICENSE("GPL v2");
