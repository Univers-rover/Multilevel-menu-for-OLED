#include "asm-generic/errno-base.h"
#include "asm-generic/gpio.h"
#include "linux/jiffies.h"
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
    {115, 0, "DHT11", },
};

/* 主设备号                                                                 */
static int major = 0;
static struct class *gpio_class;

/* 全局计时 */
static u64 g_time_list[100];
static unsigned char val[5];
static int g_isr_num = 0;
static int g_p_read = 0;
static int g_flag = 0;
static int g_isr_flag = 0;

static DECLARE_WAIT_QUEUE_HEAD(DHT11_wait);

void DHT11_start(void)
{
	int err;
	/* 将引脚设置为输出，并给一个>18ms的低电平脉冲，表示要读数据 */
	err = gpio_request(gpios[0].gpio, gpios[0].name);
	gpio_direction_output(gpios[0].gpio, 0);

	mdelay(19);

	/* 将引脚设置为输入，上拉电阻会拉为1 */
	gpio_direction_input(gpios[0].gpio);
	
}

void DHT11_end(void)
{
	/* 结束后释放中断,将引脚设置为高电平输出 */
	free_irq(gpios[0].irq, &gpios[0]);

	gpio_direction_output(gpios[0].gpio, 1);
	gpio_free(gpios[0].gpio);

	/* 将全局变量复位 */
	g_isr_flag = 0;
	g_isr_num  = 0;
	g_p_read   = 0;
}

unsigned char DHT11_rcv_one_byte( unsigned char *ptmp_data )
{
	int tmp_data = 0;
	u64 tmp_time;
	int i;

	for (i = 0; i < 8; i++) 
	{
		tmp_data <<= 1;
		tmp_time = g_time_list[g_p_read+1] - g_time_list[g_p_read];
		if ( tmp_time > 50000)
			tmp_data |= 1;
		
		g_p_read = g_p_read + 2;
	}

	*ptmp_data = tmp_data;
	//printk("rcv_one_byte\n");
	return 0;	
}

void DHT11_anasys_data(void)
{
	int i;
	g_isr_flag = 1;

	if ( g_isr_num < 81 )
	{
		//printk("lose_isr\n");
		g_flag = -1;
	}
	else
	{
		g_p_read = g_isr_num - 81;
		for (i = 0; i < 5; i++) 
		{
			if ( DHT11_rcv_one_byte( &val[i]) )
			{
				g_flag = -1;
			}
		}
	}
	
	wake_up_interruptible(&DHT11_wait);
}

static irqreturn_t DHt11_isr(int irq, void *dev_id)
{
	struct gpio_desc *gpio_desc = dev_id;
	u64 tmp_time;

	tmp_time = ktime_get_ns();
	g_time_list[g_isr_num++] = tmp_time;

	if( g_isr_num == 84 )
	{
		//printk("g_isr_num = 84\n");
		del_timer( &gpio_desc->key_timer );
		DHT11_anasys_data();
	}

	return IRQ_HANDLED;
}

/* 实现对应的open/read/write等函数，填入file_operations结构体 */
static ssize_t DHT11_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	int err;
	unsigned char tmp;
	/* 向传感器发送信号读数据 */
	DHT11_start();
	
	/* 注册中断，为后续读数据做好准备,开启定时器，开始计时 */
	err = request_irq(gpios[0].irq, DHt11_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, gpios[0].name, &gpios[0]);
	
	mod_timer(&gpios[0].key_timer, jiffies + 10);	

	/* 等待读取完毕 */
	wait_event_interruptible(DHT11_wait, g_isr_flag);

	/* 结束读取 */
	DHT11_end();
	
	/* 写回数据 */
	err = copy_to_user(buf, &val, 4);

	printk("g_flag = %d\n", g_flag);
	tmp = val[0] + val[1] + val[2] + val[3];
	if ( g_flag || tmp != val[4] )
	{
		g_flag = 0;//复位校验值
		return	-ENODATA;
	}
	g_flag = 0;//复位校验值

	return 4;
}

/* 定义自己的file_operations结构体                                              */
static struct file_operations DHT11_drv = {
	.owner	 = THIS_MODULE,
	.read   = DHT11_read,
};

static void DHT11_timer_func(unsigned long data)
{
	printk("timer_run, isr_num:%d\n",g_isr_num );
	DHT11_anasys_data();
}

/* 在入口函数 */
static int __init DHT11_init(void)
{
    int err;
    int i;
    int count = sizeof(gpios)/sizeof(gpios[0]);
    
	for (i = 0; i < count; i++)
	{
		gpios[i].irq  = gpio_to_irq(gpios[i].gpio);

		err = gpio_request(gpios[i].gpio, gpios[i].name);
		gpio_direction_output(gpios[i].gpio, 1);
		gpio_free(gpios[i].gpio);

		setup_timer(&gpios[i].key_timer, DHT11_timer_func, (unsigned long)&gpios[i]);
	}

	/* 注册file_operations 	*/
	major = register_chrdev(0, "DHT11_driver", &DHT11_drv);  /* /dev/gpio_desc */

	gpio_class = class_create(THIS_MODULE, "DHT11_driver");
	if (IS_ERR(gpio_class)) {
		//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "DHT11_driver");
		return PTR_ERR(gpio_class);
	}

	device_create(gpio_class, NULL, MKDEV(major, 0), NULL, "DHT11"); /* /dev/motor */

	return err;
}

/* 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数 */
static void __exit DHT11_exit(void)
{
	//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	device_destroy(gpio_class, MKDEV(major, 0));
	class_destroy(gpio_class);
	unregister_chrdev(major, "DHT11_driver");

}


/* 7. 其他完善：提供设备信息，自动创建设备节点                                     */

module_init(DHT11_init);
module_exit(DHT11_exit);

MODULE_LICENSE("GPL");


