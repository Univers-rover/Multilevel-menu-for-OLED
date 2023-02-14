#include "asm-generic/gpio.h"
#include "asm/gpio.h"
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/timer.h>

struct gpio_desc{
	int gpio;
	int irq;
    char *name;
    int key;
	struct timer_list key_timer;
} ;

static struct gpio_desc gpios[] = {
    {115, 0, "4988_step", },
    {116, 0, "4988_dir", },
};

/* 主设备号                                                                 */
static int major = 0;
static struct class *gpio_class;

/* 马达引脚设置数字 */

void step_4988(int step_num, int delay_time, int dir)
{
	int i;

	if(dir)
	{
		gpio_set_value(gpios[1].gpio, 1);
	}
	else
	{
		gpio_set_value(gpios[1].gpio, 0);
	}
	
	for (i= 0; i<step_num; i++)
	{
		gpio_set_value(gpios[0].gpio, 1);
		mdelay(delay_time);
		gpio_set_value(gpios[0].gpio, 0);
		mdelay(delay_time);
	}
}

/* int buf[2];
 * buf[0] = 步进的次数, > 0 : 逆时针步进; < 0 : 顺时针步进
 * buf[1] = mdelay的时间
 */
static ssize_t motor_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    int ker_buf[3];
    int err;

    if (size != 12)
        return -EINVAL;

    err = copy_from_user(ker_buf, buf, size);
    
	if (ker_buf[0] > 0)
	{
		step_4988(ker_buf[0], ker_buf[1], ker_buf[2]);
	}

    return 12;    
}

/* 定义自己的file_operations结构体                                              */
static struct file_operations gpio_key_drv = {
	.owner	 = THIS_MODULE,
	.write   = motor_write,
};

/* 在入口函数 */
static int __init motor_init(void)
{
    int err;
    int i;
    int count = sizeof(gpios)/sizeof(gpios[0]);
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	
	for (i = 0; i < count; i++)
	{
		err = gpio_request(gpios[i].gpio, gpios[i].name);
		gpio_direction_output(gpios[i].gpio, 0);
	}

	/* 注册file_operations 	*/
	major = register_chrdev(0, "4988_driver", &gpio_key_drv);  /* /dev/gpio_desc */

	gpio_class = class_create(THIS_MODULE, "4988_driver");
	if (IS_ERR(gpio_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "4988_driver");
		return PTR_ERR(gpio_class);
	}

	device_create(gpio_class, NULL, MKDEV(major, 0), NULL, "4988"); /* /dev/motor */
	
	return err;
}

/* 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数
 */
static void __exit motor_exit(void)
{
    int i;
    int count = sizeof(gpios)/sizeof(gpios[0]);
    
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	device_destroy(gpio_class, MKDEV(major, 0));
	class_destroy(gpio_class);
	unregister_chrdev(major, "4988_driver");

	for (i = 0; i < count; i++)
	{
		gpio_free(gpios[i].gpio);
	}
}


/* 7. 其他完善：提供设备信息，自动创建设备节点                                     */

module_init(motor_init);
module_exit(motor_exit);

MODULE_LICENSE("GPL");


