/*
ADC US5587 Driver
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/of.h>

#include <linux/gpio.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/spmi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/sched.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/hrtimer.h>
#include <linux/clk.h>
#include <linux/wakelock.h>

//#include <linux/i2c/ads1015.h>

//ASUS BSP Austin_T : global us5587_READY +++
bool us5587_ready;
EXPORT_SYMBOL(us5587_ready);

//Define register addresses of us5587 0X38
#define us5587_raddr 0x38

struct us5587_data
{
	u32 gpio_134;
	u32 gpio_flags134;
};

struct i2c_client *us5587_client;

/*
us5587_write_reg():	write 8 bits reg function
slave_addr:	SMBus address (7 bits)
cmd_reg   :	cmd register for programming
write_val  :	the value will be written
*/
int us5587_write_reg(uint8_t slave_addr, uint8_t cmd_reg, uint8_t write_val)
{
	int ret = 0;

	printk("[BAT][CHG] us5587_write_reg start\n");
	us5587_client->addr = slave_addr; //real SMBus address (8 bits)
	ret = i2c_smbus_write_byte_data(us5587_client, cmd_reg, write_val);
	if (ret < 0) {
		printk("%s: failed to write i2c addr=%x\n",
			__func__, slave_addr);
	}
	return ret;
}

/*
us5587_read_reg():	read 8 bits reg function
slave_addr:	SMBus address (7 bits)
cmd_reg   :	cmd register for programming
store_read_val  :	value be read will store here

*/
int us5587_read_reg(uint8_t slave_addr, uint8_t cmd_reg, uint8_t *store_read_val)
{
	int ret = 0;

	us5587_client->addr = slave_addr;
	ret = i2c_smbus_read_byte_data(us5587_client, cmd_reg);
	if (ret < 0) {
		printk("%s: failed to read i2c addr=%x\n",	__func__, slave_addr);
	}

	*store_read_val = (uint8_t) ret;

	return ret;
}

u8 us5587_adapter_value(void)
{
	u8 my_read_value = 0;

	us5587_read_reg(us5587_raddr, 0x04, &my_read_value);
	printk("[BAT][CHG] us5587_value = 0x%xh\n", my_read_value);

	return my_read_value;
}
EXPORT_SYMBOL(us5587_adapter_value);

u8 us5587_thermal_value(void)
{
	u8 my_read_value = 0;

	us5587_read_reg(us5587_raddr, 0x03, &my_read_value);
	printk("[BAT][CHG] us5587_value = 0x%xh\n", my_read_value);

	return my_read_value;
}
EXPORT_SYMBOL(us5587_thermal_value);

static ssize_t adapter_value_show(struct device *dev, struct device_attribute *da,
	char *buf)
{
	u8 val;

	val = us5587_adapter_value();

	return sprintf(buf, "0x%xh\n", val);
}

static ssize_t thermal_value_show(struct device *dev, struct device_attribute *da,
	char *buf)
{
	u16 val;

	val = us5587_thermal_value();

	return sprintf(buf, "0x%xh\n", val);
}

static ssize_t adc_ack_show(struct device *dev, struct device_attribute *da,
	char *buf)
{
	u8 val = 0;
	int ret = 0;
	bool ack = 1;

	ret = us5587_read_reg(us5587_raddr, 0x04, &val);
	if (ret < 0)
		ack = 0;
	else
		ack = 1;

	return sprintf(buf, "%d\n", ack);
}

static DEVICE_ATTR(adapter_value, 0664, adapter_value_show, NULL);
static DEVICE_ATTR(thermal_value, 0664, thermal_value_show, NULL);
static DEVICE_ATTR(adc_ack, 0664, adc_ack_show, NULL);

static struct attribute *dump_reg_attrs[] = {
	&dev_attr_adapter_value.attr,
	&dev_attr_thermal_value.attr,
	&dev_attr_adc_ack.attr,
	NULL
};


static const struct attribute_group dump_reg_attr_group = {
	.attrs = dump_reg_attrs,
};

static int us5587_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct us5587_data *data;
	int rc;

	printk("[BAT][CHG] %s start\n", __FUNCTION__);

	us5587_ready = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("[BAT][CHG] %s: i2c bus does not support the us5587\n", __FUNCTION__);
	}

	data = devm_kzalloc(&client->dev, sizeof(struct us5587_data), GFP_KERNEL);

	if (!data)
		return -ENOMEM;

	us5587_client = client;
	i2c_set_clientdata(client, data);

	rc = sysfs_create_group(&client->dev.kobj, &dump_reg_attr_group);
	if (rc)
		goto exit_remove;

	us5587_ready = 1;

	printk("[BAT][CHG] %s end\n", __FUNCTION__);

	return 0;

exit_remove:
		sysfs_remove_group(&client->dev.kobj, &dump_reg_attr_group);
	return rc;

}

static struct of_device_id us5587_match_table[] = {
	{ .compatible = "qcom,us5587",},
	{ },
};

static const struct i2c_device_id us5587_id[] = {
	{ "us5587", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, us5587_id);

static struct i2c_driver us5587_driver = {
	.driver = {
		.name = "us5587",
		.owner		= THIS_MODULE,
		.of_match_table	= us5587_match_table,
	},
	.probe = us5587_probe,
	.id_table = us5587_id,
};

module_i2c_driver(us5587_driver);

MODULE_AUTHOR("Dirk Eibach <eibach@gdsys.de>");
MODULE_DESCRIPTION("US5587 driver");
MODULE_LICENSE("GPL");

