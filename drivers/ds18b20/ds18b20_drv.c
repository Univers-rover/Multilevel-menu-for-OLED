#include "asm-generic/errno-base.h"
#include "asm-generic/gpio.h"
#include <linux/string.h>
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

#define GetBit(x,y) ((x) >> (y)&1)
#define CMD_INIT 0
#define CMD_F0H  1
#define CMD_33H  2
#define CMD_55H  3
#define CMD_CCH  4
#define CMD_ECH  5
#define CMD_44H  6
#define CMD_4EH  7
#define CMD_BEH  8
#define CMD_48H  9
#define CMD_B8H 10
#define CMD_B4H 11

struct gpio_desc{
	int gpio;
	int irq;
    char *name;
    int key;
	struct timer_list key_timer;
} ;

static struct gpio_desc gpios[] = {
    {115, 0, "ds18b20", },
};

/* 主设备号                                                                 */
static int major = 0;
static struct class *gpio_class;
static spinlock_t ds18b20_spinlock;

/* 用于初始化ds18b20时序，成功返回0，失败返回-1 */
static void ds18b20_udelay(int us)
{
	u64 time = ktime_get_ns();
	while ( (ktime_get_ns() - time) < us*1000);
}

static int ds18b20_chip_init(void)
{
	int count = 0;

	gpio_direction_output(gpios[0].gpio, 0);
	ds18b20_udelay(490);
	gpio_direction_input(gpios[0].gpio);

	/* 读不到0就1us读一次，最多读100次 */
	while ( gpio_get_value(gpios[0].gpio) && (count++ < 100) )
	{
		ds18b20_udelay(1);
	}
	/* 没有回应直接返回 */
	if ( count == 100 )
		return -1;

	/* 如果有回应，等待ds18b20回应结束，避免回应信号影响后续通信 */
	count = 0;
	while (!gpio_get_value( gpios[0].gpio) && (count++ < 300) )
	{
		ds18b20_udelay(1);
	}
	if ( count == 300 )
		return -2;

	return 0;
}

/* 向传感器写入一个bit,输入为0或1 */
static void ds18b20_write_bit(int bit)
{
	if ( bit )//写1
	{
		gpio_direction_output(gpios[0].gpio, 0);
		ds18b20_udelay(5);
		gpio_direction_output(gpios[0].gpio, 1);
		ds18b20_udelay(60);
	}
	else//写0
	{
		gpio_direction_output(gpios[0].gpio, 0);
		ds18b20_udelay(65);
		gpio_direction_output(gpios[0].gpio, 1);
	}
}

/* 向传感器写入一个byte,输入为0或1,从低位到高位 */
static void ds18b20_write_byte(unsigned char byte)
{
	int i;

	for (i = 0; i < 8 ; i++)
	{
		ds18b20_write_bit(GetBit(byte, i));
	}
}

/* 从buffer中读取一个byte，从低位到高位，成功返回0，失败返回-1 */
static int ds18b20_read_byte(unsigned char *data)
{
	int i;
	unsigned char tmp_data = 0;
	int val;

	for (i = 0; i < 8; i++)
	{
		gpio_direction_output(gpios[0].gpio, 0);
		ds18b20_udelay(5);
		gpio_direction_input(gpios[0].gpio);//在15us内读取数据
		ds18b20_udelay(10);
		val = gpio_get_value(gpios[0].gpio);
		if (val)
		{
			tmp_data |= (1<<i);
		}	
		ds18b20_udelay(50);
		gpio_direction_output(gpios[0].gpio, 1);
	}

	*data = tmp_data;
	return 0;
}

/* crc校验函数 */
static unsigned char calcrc_1byte(unsigned char abyte)   
{   
	unsigned char i,crc_1byte;     
	crc_1byte=0;                //设定crc_1byte初值为0  
	for(i = 0; i < 8; i++)   
	{   
		if( ( (crc_1byte^abyte)&0x01 ) )   
		{   
			crc_1byte^=0x18;     
			crc_1byte>>=1;   
			crc_1byte|=0x80;   
		}         
		else     
			crc_1byte>>=1;   

		abyte>>=1;         
	}   
	return crc_1byte;   
}

/* crc校验函数 */
static unsigned char calcrc_bytes(unsigned char *p,unsigned char len)  
{  
	unsigned char crc = 0;  
	while(len--) //len为总共要校验的字节数 
	{  
		crc=calcrc_1byte(crc^*p++);  
	}  
	return crc;  //若最终返回的crc为0，则数据传输正确  
}

/* ds18b20的crc校验函数,正确返回0，错误返回-1 */
static int ds18b20_verify_crc(unsigned char *buf)
{
    unsigned char crc;

	crc = calcrc_bytes(buf, 8);

    if (crc == buf[8])
		return 0;
	else
		return -1;
}

static void ds18b20_calc_value(unsigned char *data, int *result)
{
	unsigned char temper_L, temper_H;//代表温度的byte0和byte1，byte0是低8位，高8位中分辨正负
	unsigned int integer;
	unsigned int decimal, decimal1, decimal2;
	int negative = 1;

	temper_L = *(data);
	temper_H = *(data+1);

	if ( temper_H > 0x7F ){
		negative = -1;
		temper_L = ~temper_L + 1;
		temper_H = ~temper_H;
	}

	integer  = (temper_H << 4) | (temper_L >> 4);
	decimal1 = (temper_L&0x0F) * 10 / 16;
	decimal2 = (temper_L&0x0F) * 100 / 16 % 10;
	decimal  = decimal1 * 10 + decimal2;
	//printk("integer:%d",integer);
	*(result) = integer * negative;
	*(result + 1) = decimal;
}

static ssize_t ds18b20_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    unsigned char ker_buf[8];
	int i;
    int err;

    if (size != 8)
        return -EINVAL;

    err = copy_from_user(ker_buf, buf, size);
	for (i = 0; i < size; i++ )
	{
		ds18b20_write_byte(ker_buf[i]);
	}
    
    return 8;
}

/* 实现对应的open/read/write等函数，填入file_operations结构体 */
static ssize_t ds18b20_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	int i;
	int err = 0;
	unsigned long flags;
	unsigned char temp[9];
	int temperature[2];

	/* 1 启动温度转换 */
	/* 1.1 为了避免过程中被干扰,关中断 */
	spin_lock_irqsave(&ds18b20_spinlock, flags);

	/* 1.2 初始化芯片 */
	err = ds18b20_chip_init();
	if (err)
	{
		spin_unlock_irqrestore(&ds18b20_spinlock, flags);
		printk("ds18b20_init err:%d\n",err);
		return -1;
	}

	/* 1.3 发出命令: skip rom, 0xcc */
	ds18b20_write_byte(0xCC);

	/* 1.4 发出命令: 启动温度转换, 0x44 */
	ds18b20_write_byte(0x44);

	/* 1.5 恢复中断 */
	spin_unlock_irqrestore(&ds18b20_spinlock, flags);

	/* 2 等待温度转换成功 : 可能长达1s,又不能长时间占用cpu资源 */
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(msecs_to_jiffies(1000));

	/* 3 读取数据 */
	/* 3.1 关中断 */
	spin_lock_irqsave(&ds18b20_spinlock, flags);

	/* 3.2 初始化芯片 */
	err = ds18b20_chip_init();
	if (err)
	{
		spin_unlock_irqrestore(&ds18b20_spinlock, flags);
		printk("ds18b20_init err:%d\n",err);
		return -1;
	}

	/* 3.3 发送命令：skip rom, 0xcc */
	ds18b20_write_byte(0xCC);

	/* 3.4 发送命令：Read Scratchpad, 0xbe */
	ds18b20_write_byte(0xBE);

	/* 3.5 读取数据 */
	for ( i = 0; i < 9; i++ )
	{
		if ( ds18b20_read_byte(&temp[i]) )
			return -EINVAL;
	}
	
	/* 3.6 恢复中断 */
	spin_unlock_irqrestore(&ds18b20_spinlock, flags);

	printk("data_low:%x  data_high:%x\n",temp[0], temp[1]);//低8位，高8位

	/* 3.7 对数据进行crc校验 */
	if ( ds18b20_verify_crc(temp) )
		return -EINVAL;

	printk("verify_crc ok\n");

	/* 4 计算温度值并返回给用户 */
	ds18b20_calc_value(temp, temperature);
	printk("integer:%d",temperature[0]);
	err = copy_to_user(buf, temperature, size);

	return 8;
}

// ioctl(fd, CMD, ARG) 成功返回0，失败返回-1
static long ds18b20_ioctl(struct file *filp, unsigned int command, unsigned long arg)
{
	int ret = 0;
	switch (command)
	{
		/* 初始化命令，失败返回-1 */
		case CMD_INIT:
		{
			if ( ds18b20_chip_init() )
				ret = -1;
			return ret;
		}
		case CMD_F0H:{
			ds18b20_write_byte(0xF0);
		}
		case CMD_33H:{
			ds18b20_write_byte(0x33);
		}
		case CMD_55H:{
			ds18b20_write_byte(0x55);
		}
		case CMD_CCH:{
			ds18b20_write_byte(0xCC);
		}
		case CMD_ECH:{
			ds18b20_write_byte(0xEC);
		}
		case CMD_BEH:{
			ds18b20_write_byte(0xBE);
		}
		case CMD_48H:{
			ds18b20_write_byte(0x48);
		}
		case CMD_B8H:{
			ds18b20_write_byte(0xB8);
		}
		case CMD_B4H:{
			ds18b20_write_byte(0xB4);
		}
	}

	return ret;
}

/* 定义自己的file_operations结构体                                              */
static struct file_operations ds18b20_drv = {
	.owner	= THIS_MODULE,
	.write	= ds18b20_write,
	.read   = ds18b20_read,
	.unlocked_ioctl = ds18b20_ioctl,
};

/* 在入口函数 */
static int __init ds18b20_init(void)
{
    int err;
    int i;
    int count = sizeof(gpios)/sizeof(gpios[0]);

	spin_lock_init(&ds18b20_spinlock);
    
	for (i = 0; i < count; i++)
	{
		err = gpio_request(gpios[i].gpio, gpios[i].name);
		gpio_direction_output(gpios[i].gpio, 1);
	}

	/* 注册file_operations 	*/
	major = register_chrdev(0, "ds18b20_driver", &ds18b20_drv);  /* /dev/gpio_desc */

	gpio_class = class_create(THIS_MODULE, "ds18b20_driver");
	if (IS_ERR(gpio_class)) {
		//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "ds18b20_driver");
		return PTR_ERR(gpio_class);
	}

	device_create(gpio_class, NULL, MKDEV(major, 0), NULL, "ds18b20"); /* /dev/motor */

	return err;
}

/* 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数 */
static void __exit ds18b20_exit(void)
{
	int i;
	int count = sizeof(gpios)/sizeof(gpios[0]);
	//printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

	device_destroy(gpio_class, MKDEV(major, 0));
	class_destroy(gpio_class);
	unregister_chrdev(major, "ds18b20_driver");

	for (i = 0; i < count; i++)
	{
		gpio_free(gpios[i].gpio);
	}

}

/* 7. 其他完善：提供设备信息，自动创建设备节点                                     */

module_init(ds18b20_init);
module_exit(ds18b20_exit);

MODULE_LICENSE("GPL");


