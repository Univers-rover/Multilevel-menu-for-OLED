#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/list.h>
#include <linux/spi/spi.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>

#define OLED_SET_XY	  		99
#define OLED_SET_XY_WRITE_DATA  100
#define OLED_SET_XY_WRITE_DATAS 101
#define OLED_SET_DATAS	  		102  /* 102为低8位, 高16位用来表示长度 */

//为0 表示命令，为1表示数据
#define OLED_CMD 	0
#define OLED_DATA 	1

struct spi_device *oled_dev;
static struct gpio_desc *oled_dc;
static unsigned char *data_buf;

static unsigned int pseudo_palette[16];

static struct fb_info *oled_fb_info;

static struct task_struct *oled_kthread;

static int mylcd_setcolreg(unsigned regno,
			       unsigned red, unsigned green, unsigned blue,
			       unsigned transp, struct fb_info *info)
{
	return 1; /* unkown type */
}

static struct fb_ops myfb_ops = {
	.owner		= THIS_MODULE,
	.fb_setcolreg	= mylcd_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

/**********************************************************************
	 * 函数名称： oled_write_cmd
	 * 功能描述： oled向特定地址写入数据或者命令
	 * 输入参数：@ uc_data :要写入的数据
	 			@ uc_cmd:为1则表示写入数据，为0表示写入命令
	 * 输出参数：无
	 * 返 回 值： 无
	 * 修改日期 	   版本号	 修改人		  修改内容
	 * -----------------------------------------------
	 * 2020/03/04		 V1.0	  芯晓		  创建
 ***********************************************************************/
static void oled_write_cmd_data(unsigned char uc_data,unsigned char uc_cmd)
{
	if(uc_cmd==0)
	{
		//*GPIO4_DR_s &= ~(1<<20);//拉低，表示写入指令
		gpiod_set_value(oled_dc, 0);
	}
	else
	{
		//*GPIO4_DR_s |= (1<<20);//拉高，表示写入数据
		gpiod_set_value(oled_dc, 1);
	}
	// spi_writeread(ESCPI1_BASE,uc_data);//写入
	spi_write(oled_dev, &uc_data, 1);
}

static void oled_write_datas(unsigned char *buf, int len)
{
	int err;
	//*GPIO4_DR_s |= (1<<20);//拉高，表示写入数据
	gpiod_set_value(oled_dc, 1);
	// spi_writeread(ESCPI1_BASE,uc_data);//写入
	err = spi_write(oled_dev, buf, len);
}

/**********************************************************************
	 * 函数名称： oled_init
	 * 功能描述： oled_init的初始化，包括SPI控制器得初始化
	 * 输入参数：无
	 * 输出参数： 初始化的结果
	 * 返 回 值： 成功则返回0，否则返回-1
	 * 修改日期 	   版本号	 修改人		  修改内容
	 * -----------------------------------------------
	 * 2020/03/15		 V1.0	  芯晓		  创建
 ***********************************************************************/
static int oled_hardware_init(void)
{		  			 		  						  					  				 	   		  	  	 	  
	oled_write_cmd_data(0xae,OLED_CMD);//关闭显示

	oled_write_cmd_data(0x00,OLED_CMD);//设置 lower column address
	oled_write_cmd_data(0x10,OLED_CMD);//设置 higher column address

	oled_write_cmd_data(0x40,OLED_CMD);//设置 display start line

	oled_write_cmd_data(0xB0,OLED_CMD);//设置page address

	oled_write_cmd_data(0x81,OLED_CMD);// contract control
	oled_write_cmd_data(0x66,OLED_CMD);//128

	oled_write_cmd_data(0xa1,OLED_CMD);//设置 segment remap

	oled_write_cmd_data(0xa6,OLED_CMD);//normal /reverse

	oled_write_cmd_data(0xa8,OLED_CMD);//multiple ratio
	oled_write_cmd_data(0x3f,OLED_CMD);//duty = 1/64

	oled_write_cmd_data(0xc8,OLED_CMD);//com scan direction

	oled_write_cmd_data(0xd3,OLED_CMD);//set displat offset
	oled_write_cmd_data(0x00,OLED_CMD);//

	oled_write_cmd_data(0xd5,OLED_CMD);//set osc division
	oled_write_cmd_data(0x80,OLED_CMD);//

	oled_write_cmd_data(0xd9,OLED_CMD);//ser pre-charge period
	oled_write_cmd_data(0x1f,OLED_CMD);//

	oled_write_cmd_data(0xda,OLED_CMD);//set com pins
	oled_write_cmd_data(0x12,OLED_CMD);//

	oled_write_cmd_data(0xdb,OLED_CMD);//set vcomh
	oled_write_cmd_data(0x30,OLED_CMD);//

	oled_write_cmd_data(0x8d,OLED_CMD);//set charge pump disable 
	oled_write_cmd_data(0x14,OLED_CMD);//

	oled_write_cmd_data(0xaf,OLED_CMD);//set dispkay on

	return 0;
}

//坐标设置
/**********************************************************************
	 * 函数名称： OLED_DIsp_Set_Pos
	 * 功能描述：设置要显示的位置
	 * 输入参数：@ x ：要显示的column address
	 			@y :要显示的page address
	 * 输出参数： 无
	 * 返 回 值： 
	 * 修改日期 	   版本号	 修改人		  修改内容
	 * -----------------------------------------------
	 * 2020/03/15		 V1.0	  芯晓		  创建
 ***********************************************************************/
void OLED_DIsp_Set_Pos(int x, int y)
{ 	
	oled_write_cmd_data(0xb0+y,OLED_CMD);
	oled_write_cmd_data((x&0x0f),OLED_CMD); 
	oled_write_cmd_data(((x&0xf0)>>4)|0x10,OLED_CMD);
}   	      	   			 

/**********************************************************************
	 * 函数名称： OLED_DIsp_Clear
	 * 功能描述： 整个屏幕显示数据清0
	 * 输入参数：无
	 * 输出参数： 无
	 * 返 回 值： 
	 * 修改日期 	   版本号	 修改人		  修改内容
	 * -----------------------------------------------
	 * 2020/03/15		 V1.0	  芯晓		  创建
 ***********************************************************************/
static void OLED_DIsp_Clear(void)  
{
    unsigned char x, y;
    for (y = 0; y < 8; y++)
    {
        OLED_DIsp_Set_Pos(0, y);
        for (x = 0; x < 128; x++)
            oled_write_cmd_data((y < 4)?0:0xff, OLED_DATA); /* 清零 */
    }
}

static int get_pixel(int x, int y)
{
	unsigned char byte = *(oled_fb_info->screen_base + y*oled_fb_info->fix.line_length + (x>>3));
	int bit = x & 0x7;
	if (byte & (1 << (7-bit)))
		return 1;
	else 
		return 0;
}

static void convert_fb_to_oled(void)
{
	unsigned char data;
	int i = 0;
	int x, page;
	int bit;

	for (page = 0; page < 8; page++)
	{
		for (x = 0; x < 128; x++)
		{
			data = 0;
			for (bit = 0; bit < 8; bit++)
			{
				data |= (get_pixel(x, page*8 + bit) << bit);
			}
			data_buf[i++] = data;
		}
	}
}

//利用内和线程每秒刷新一次
static int oled_thread(void *data)
{
    unsigned char y;
	while (1)
	{
		/* 把Framebuffer的数据刷到OLED上去 */
		convert_fb_to_oled();
		//memcpy(data_buf, oled_fb_info->screen_base, 1024);
		for (y = 0; y < 8; y++)
		{
			OLED_DIsp_Set_Pos(0, y);
			oled_write_datas(&data_buf[y*128], 128);
			//oled_write_datas(oled_fb_info->screen_base+y*128, 128);
		}
		
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ/60);//延时多久刷新一次，HZ是1s

		if (kthread_should_stop()) {
			set_current_state(TASK_RUNNING);
			break;
		}
	}
	return 0;
}


static int oled_probe(struct spi_device *spi)
{	
	dma_addr_t phy_addr;
	
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	oled_dev = spi;

	/* 分配/设置/注册 fb_info */
	oled_fb_info = framebuffer_alloc(0, &spi->dev);

	/* 1.2 设置fb_info */
	/* a. var : LCD分辨率、颜色格式 */
	oled_fb_info->var.xres_virtual = oled_fb_info->var.xres = 128;
	oled_fb_info->var.yres_virtual = oled_fb_info->var.yres = 64;
	
	oled_fb_info->var.bits_per_pixel = 1;  /* rgb565 */
	
	/* b. fix */
	strcpy(oled_fb_info->fix.id, "100ask_lcd");
	oled_fb_info->fix.smem_len = oled_fb_info->var.xres * oled_fb_info->var.yres * oled_fb_info->var.bits_per_pixel / 8;

	/* c. 分配显存 */
	oled_fb_info->screen_base = dma_alloc_wc(NULL, oled_fb_info->fix.smem_len, &phy_addr,
					 GFP_KERNEL);
	oled_fb_info->fix.smem_start = phy_addr;  /* fb的物理地址 */

	oled_fb_info->fix.type = FB_TYPE_PACKED_PIXELS;
	oled_fb_info->fix.visual = FB_VISUAL_MONO10;//表明是黑白屏

	oled_fb_info->fix.line_length = oled_fb_info->var.xres * oled_fb_info->var.bits_per_pixel / 8;

	/* d. fbops */
	oled_fb_info->fbops = &myfb_ops;
	oled_fb_info->pseudo_palette = pseudo_palette;	

	register_framebuffer(oled_fb_info);

	/* spi oled init */
	oled_dc = gpiod_get(&spi->dev, "dc", GPIOD_OUT_HIGH);
	
	//dma_alloc_get可以确保申请的内存物理地址连续
	data_buf = dma_alloc_wc(NULL, oled_fb_info->fix.smem_len, &phy_addr, GFP_KERNEL);
	
	oled_hardware_init();

	OLED_DIsp_Clear();

	/* 创建1个内核线程,用来把Framebuffer的数据通过SPI控制器发送给OLED */
	oled_kthread = kthread_create(oled_thread, NULL, "100ask_oled");
	wake_up_process(oled_kthread);
	
	return 0;
}

static int oled_remove(struct spi_device *spi)
{
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

	/* 反注册 fb_info */
	unregister_framebuffer(oled_fb_info);

	gpiod_put(oled_dc);

	return 0;
}

//static const struct spi_device_id oled_spi_ids[] = {
//	{"100ask,oled", },
//	{}
//};

static const struct of_device_id oled_of_match[] = {
	{.compatible = "100ask,oled"},
	{}
};

static struct spi_driver oled_driver = {
	.driver = {
		.name	= "oled",
		.of_match_table = oled_of_match,
	},
	.probe		= oled_probe,
	.remove		= oled_remove,
	//.id_table	= oled_spi_ids,
};

int oled_init(void)
{
	return spi_register_driver(&oled_driver);
}

static void oled_exit(void)
{
	spi_unregister_driver(&oled_driver);
}

module_init(oled_init);
module_exit(oled_exit);

MODULE_LICENSE("GPL v2");

